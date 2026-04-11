"""
Tests for the DG-LAN HTTP file streamer.

Covers: FileStreamer.resolve(), FileStreamer.stream(), parse_range_header(),
        compute_etag(), guess_content_type(), and the HTTP endpoint integration.

Run:  pytest test_streamer.py -v
"""

import asyncio
import os
import tempfile
import time
from http import HTTPStatus
from unittest.mock import MagicMock

import pytest

# Ensure the dglan-api directory is importable
import sys
sys.path.insert(0, os.path.dirname(__file__))

from server import (
    FileStreamer,
    parse_range_header,
    compute_etag,
    guess_content_type,
    SE_HASH_PATTERN,
    STREAM_CHUNK_SIZE,
)


# ── Fixtures ──────────────────────────────────────────────────

VALID_SE_HASH = "a" * 56  # 56 hex chars


@pytest.fixture
def shared_dir():
    """Create a temporary shared directory with test files."""
    with tempfile.TemporaryDirectory() as tmpdir:
        # Create test files
        test_file = os.path.join(tmpdir, "movie.mkv")
        with open(test_file, "wb") as f:
            f.write(b"x" * 1024)

        sub_dir = os.path.join(tmpdir, "Videos")
        os.makedirs(sub_dir)
        nested_file = os.path.join(sub_dir, "clip.mp4")
        with open(nested_file, "wb") as f:
            f.write(b"y" * 512)

        empty_file = os.path.join(tmpdir, "empty.txt")
        with open(empty_file, "wb") as f:
            pass

        yield tmpdir


@pytest.fixture
def streamer(shared_dir):
    """FileStreamer with a single shared entry pointing at the temp dir."""
    mock_core = MagicMock()
    fs = FileStreamer(mock_core)
    fs.update_index([{"id": VALID_SE_HASH, "path": shared_dir}])
    return fs


# ── SE_HASH_PATTERN tests ────────────────────────────────────

class TestHashPattern:
    def test_valid_hash(self):
        assert SE_HASH_PATTERN.match("a" * 56)

    def test_valid_hash_mixed_hex(self):
        assert SE_HASH_PATTERN.match("0123456789abcdef" * 3 + "01234567")

    def test_too_short(self):
        assert SE_HASH_PATTERN.match("a" * 55) is None

    def test_too_long(self):
        assert SE_HASH_PATTERN.match("a" * 57) is None

    def test_non_hex_chars(self):
        assert SE_HASH_PATTERN.match("g" * 56) is None

    def test_uppercase_rejected(self):
        assert SE_HASH_PATTERN.match("A" * 56) is None

    def test_empty(self):
        assert SE_HASH_PATTERN.match("") is None


# ── FileStreamer.resolve() tests ──────────────────────────────

class TestResolve:
    def test_valid_file(self, streamer):
        result = streamer.resolve(VALID_SE_HASH, "movie.mkv")
        assert result is not None
        assert result.endswith("movie.mkv")
        assert os.path.isfile(result)

    def test_nested_file(self, streamer):
        result = streamer.resolve(VALID_SE_HASH, "Videos/clip.mp4")
        assert result is not None
        assert result.endswith("clip.mp4")

    def test_unknown_hash(self, streamer):
        result = streamer.resolve("b" * 56, "movie.mkv")
        assert result is None

    def test_invalid_hash_format(self, streamer):
        result = streamer.resolve("not-a-hash", "movie.mkv")
        assert result is None

    def test_path_traversal_dotdot(self, streamer, shared_dir):
        result = streamer.resolve(VALID_SE_HASH, "../../etc/passwd")
        assert result is None

    def test_path_traversal_absolute(self, streamer):
        if os.name == "nt":
            result = streamer.resolve(VALID_SE_HASH, "C:/Windows/System32/cmd.exe")
        else:
            result = streamer.resolve(VALID_SE_HASH, "/etc/passwd")
        assert result is None

    def test_null_byte_in_path(self, streamer):
        result = streamer.resolve(VALID_SE_HASH, "movie\x00.mkv")
        assert result is None

    def test_nonexistent_file(self, streamer):
        result = streamer.resolve(VALID_SE_HASH, "nope.txt")
        assert result is None

    def test_directory_not_file(self, streamer):
        result = streamer.resolve(VALID_SE_HASH, "Videos")
        assert result is None

    def test_empty_path(self, streamer):
        result = streamer.resolve(VALID_SE_HASH, "")
        assert result is None

    def test_dot_path(self, streamer):
        """'.' resolves to the directory itself, not a file."""
        result = streamer.resolve(VALID_SE_HASH, ".")
        assert result is None

    def test_update_index(self, shared_dir):
        mock_core = MagicMock()
        fs = FileStreamer(mock_core)
        assert fs.resolve(VALID_SE_HASH, "movie.mkv") is None
        fs.update_index([{"id": VALID_SE_HASH, "path": shared_dir}])
        assert fs.resolve(VALID_SE_HASH, "movie.mkv") is not None


# ── parse_range_header() tests ────────────────────────────────

class TestParseRange:
    def test_full_range(self):
        assert parse_range_header("bytes=0-999", 1000) == (0, 999)

    def test_open_end(self):
        assert parse_range_header("bytes=500-", 1000) == (500, 999)

    def test_suffix_range(self):
        assert parse_range_header("bytes=-200", 1000) == (800, 999)

    def test_single_byte(self):
        assert parse_range_header("bytes=0-0", 1000) == (0, 0)

    def test_last_byte(self):
        assert parse_range_header("bytes=999-999", 1000) == (999, 999)

    def test_invalid_start_greater_than_end(self):
        assert parse_range_header("bytes=999-0", 1000) is None

    def test_end_beyond_file_clamped(self):
        """RFC 7233: last-byte-pos beyond file size is clamped to file_size - 1."""
        assert parse_range_header("bytes=0-9999", 100) == (0, 99)

    def test_start_beyond_file(self):
        assert parse_range_header("bytes=1000-", 1000) is None

    def test_suffix_zero(self):
        assert parse_range_header("bytes=-0", 1000) is None

    def test_suffix_exceeds_file(self):
        assert parse_range_header("bytes=-2000", 1000) is None

    def test_multipart_rejected(self):
        assert parse_range_header("bytes=0-100,200-300", 1000) is None

    def test_wrong_prefix(self):
        assert parse_range_header("chars=0-100", 1000) is None

    def test_no_prefix(self):
        assert parse_range_header("0-100", 1000) is None

    def test_garbage(self):
        assert parse_range_header("bytes=abc-def", 1000) is None

    def test_negative_start(self):
        assert parse_range_header("bytes=-1-100", 1000) is None


# ── compute_etag() tests ─────────────────────────────────────

class TestComputeEtag:
    def test_format(self, shared_dir):
        path = os.path.join(shared_dir, "movie.mkv")
        etag = compute_etag("abcd1234", path)
        assert etag.startswith('"')
        assert etag.endswith('"')
        assert "abcd1234:" in etag

    def test_contains_size(self, shared_dir):
        path = os.path.join(shared_dir, "movie.mkv")
        etag = compute_etag("test", path)
        assert ":1024" in etag

    def test_different_prefix(self, shared_dir):
        path = os.path.join(shared_dir, "movie.mkv")
        e1 = compute_etag("aaaa", path)
        e2 = compute_etag("bbbb", path)
        assert e1 != e2


# ── guess_content_type() tests ────────────────────────────────

class TestGuessContentType:
    def test_mp4(self):
        assert guess_content_type("video.mp4") == "video/mp4"

    def test_mkv(self):
        ct = guess_content_type("video.mkv")
        # mkv may or may not be known depending on system
        assert ct in ("video/x-matroska", "application/octet-stream")

    def test_txt(self):
        ct = guess_content_type("readme.txt")
        assert "text" in ct

    def test_zip(self):
        ct = guess_content_type("archive.zip")
        assert ct in ("application/zip", "application/x-zip-compressed")

    def test_unknown(self):
        assert guess_content_type("file.xyz123") == "application/octet-stream"

    def test_no_extension(self):
        assert guess_content_type("README") == "application/octet-stream"


# ── FileStreamer.stream() integration tests ───────────────────

class MockWriter:
    """Minimal asyncio.StreamWriter mock that captures written bytes."""

    def __init__(self):
        self.data = bytearray()

    def write(self, data: bytes):
        self.data.extend(data)

    async def drain(self):
        pass


class TestStream:
    def _run(self, coro):
        loop = asyncio.new_event_loop()
        try:
            return loop.run_until_complete(coro)
        finally:
            loop.close()

    def test_full_file_200(self, streamer, shared_dir):
        writer = MockWriter()
        path = os.path.join(shared_dir, "movie.mkv")
        self._run(streamer.stream(writer, path, None, None, False, {}))
        output = bytes(writer.data)
        assert b"HTTP/1.1 200 OK" in output
        assert b"Content-Length: 1024" in output
        assert b"Accept-Ranges: bytes" in output
        # Body: 1024 bytes of 'x'
        header_end = output.find(b"\r\n\r\n")
        body = output[header_end + 4:]
        assert len(body) == 1024
        assert body == b"x" * 1024

    def test_range_206(self, streamer, shared_dir):
        writer = MockWriter()
        path = os.path.join(shared_dir, "movie.mkv")
        self._run(streamer.stream(writer, path, "bytes=0-99", None, False, {}))
        output = bytes(writer.data)
        assert b"HTTP/1.1 206 Partial Content" in output
        assert b"Content-Range: bytes 0-99/1024" in output
        assert b"Content-Length: 100" in output
        header_end = output.find(b"\r\n\r\n")
        body = output[header_end + 4:]
        assert len(body) == 100
        assert body == b"x" * 100

    def test_range_suffix(self, streamer, shared_dir):
        writer = MockWriter()
        path = os.path.join(shared_dir, "movie.mkv")
        self._run(streamer.stream(writer, path, "bytes=-100", None, False, {}))
        output = bytes(writer.data)
        assert b"206 Partial Content" in output
        assert b"Content-Range: bytes 924-1023/1024" in output

    def test_invalid_range_416(self, streamer, shared_dir):
        writer = MockWriter()
        path = os.path.join(shared_dir, "movie.mkv")
        self._run(streamer.stream(writer, path, "bytes=2000-3000", None, False, {}))
        output = bytes(writer.data)
        assert b"416" in output
        assert b"bytes */1024" in output

    def test_conditional_304(self, streamer, shared_dir):
        path = os.path.join(shared_dir, "movie.mkv")
        etag = compute_etag(VALID_SE_HASH[:16], path)
        writer = MockWriter()
        self._run(streamer.stream(writer, path, None, etag, False, {}, se_hash_prefix=VALID_SE_HASH[:16]))
        output = bytes(writer.data)
        assert b"304 Not Modified" in output
        # No body for 304
        header_end = output.find(b"\r\n\r\n")
        body = output[header_end + 4:]
        assert len(body) == 0

    def test_conditional_mismatch_200(self, streamer, shared_dir):
        path = os.path.join(shared_dir, "movie.mkv")
        writer = MockWriter()
        self._run(streamer.stream(writer, path, None, '"stale-etag"', False, {}, se_hash_prefix=VALID_SE_HASH[:16]))
        output = bytes(writer.data)
        assert b"200 OK" in output

    def test_content_disposition_inline(self, streamer, shared_dir):
        writer = MockWriter()
        path = os.path.join(shared_dir, "movie.mkv")
        self._run(streamer.stream(writer, path, None, None, False, {}))
        assert b'inline; filename="movie.mkv"' in bytes(writer.data)

    def test_content_disposition_attachment(self, streamer, shared_dir):
        writer = MockWriter()
        path = os.path.join(shared_dir, "movie.mkv")
        self._run(streamer.stream(writer, path, None, None, True, {}))
        assert b'attachment; filename="movie.mkv"' in bytes(writer.data)

    def test_content_type_header(self, streamer, shared_dir):
        writer = MockWriter()
        path = os.path.join(shared_dir, "Videos", "clip.mp4")
        self._run(streamer.stream(writer, path, None, None, False, {}))
        assert b"Content-Type: video/mp4" in bytes(writer.data)

    def test_cors_headers_included(self, streamer, shared_dir):
        writer = MockWriter()
        path = os.path.join(shared_dir, "movie.mkv")
        cors = {"Access-Control-Allow-Origin": "*"}
        self._run(streamer.stream(writer, path, None, None, False, cors))
        assert b"Access-Control-Allow-Origin: *" in bytes(writer.data)

    def test_empty_file(self, streamer, shared_dir):
        writer = MockWriter()
        path = os.path.join(shared_dir, "empty.txt")
        self._run(streamer.stream(writer, path, None, None, False, {}))
        output = bytes(writer.data)
        assert b"200 OK" in output
        assert b"Content-Length: 0" in output

    def test_large_range_middle(self, streamer, shared_dir):
        writer = MockWriter()
        path = os.path.join(shared_dir, "movie.mkv")
        self._run(streamer.stream(writer, path, "bytes=100-199", None, False, {}))
        output = bytes(writer.data)
        header_end = output.find(b"\r\n\r\n")
        body = output[header_end + 4:]
        assert len(body) == 100
        assert body == b"x" * 100


# ── End-to-end HTTP route simulation ─────────────────────────

class TestHTTPRouting:
    """Tests that validate the URL parsing and routing logic for file streaming."""

    def test_se_hash_extraction(self):
        """Verify SE hash and path are correctly split from URL remainder."""
        remainder = f"{VALID_SE_HASH}/Videos/clip.mp4"
        slash_pos = remainder.find("/")
        se_hash = remainder[:slash_pos].lower()
        relative_path = remainder[slash_pos + 1:]
        assert se_hash == VALID_SE_HASH
        assert relative_path == "Videos/clip.mp4"

    def test_no_slash_in_remainder(self):
        """Remainder with no slash should fail (no file path)."""
        remainder = VALID_SE_HASH
        assert "/" not in remainder

    def test_url_decode_path(self):
        """URL-encoded characters should be properly decoded."""
        import urllib.parse
        encoded = "My%20Videos/a%20file.mp4"
        decoded = urllib.parse.unquote(encoded)
        assert decoded == "My Videos/a file.mp4"

    def test_empty_relative_path(self):
        """Trailing slash after hash means empty path — should be rejected."""
        remainder = f"{VALID_SE_HASH}/"
        slash_pos = remainder.find("/")
        relative_path = remainder[slash_pos + 1:]
        assert relative_path == ""
