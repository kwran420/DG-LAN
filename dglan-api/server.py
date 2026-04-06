"""
DG-LAN API Server
=================
Standalone HTTP server that connects to a local DG-LAN Core daemon,
reads shared files with their hashes, and serves them as JSON.

Your website fetches /api/files to build dglan:// download links.

Usage:
    python server.py                         # defaults: Core on 127.0.0.1:59485, HTTP on 0.0.0.0:8080
    python server.py --core-host 10.0.0.5    # remote Core
    python server.py --http-port 9090        # different HTTP port
    python server.py --password letmein      # if Core has a remote password set
"""

import argparse
import asyncio
import hashlib
import json
import logging
import struct
import time
import urllib.parse
from http import HTTPStatus
from typing import Optional

import common_pb2 as common
import gui_protocol_pb2 as gui

log = logging.getLogger("dglan-api")

# ── Wire format constants ────────────────────────────────────
HASH_SIZE = 28
HEADER_SIZE = 4 + 4 + HASH_SIZE  # type(u32) + size(u32) + sender_id(28) = 36

# Message type codes (from MessageHeader.h)
GUI_STATE                  = 0x1001
GUI_STATE_RESULT           = 0x1002
GUI_ASK_FOR_AUTHENTICATION = 0x1021
GUI_AUTHENTICATION         = 0x1022
GUI_AUTHENTICATION_RESULT  = 0x1023
GUI_BROWSE                 = 0x1051
GUI_BROWSE_TAG             = 0x1052
GUI_BROWSE_RESULT          = 0x1053
GUI_REFRESH                = 0x10A1


def hash_to_hex(raw: bytes) -> str:
    """Convert 28-byte raw hash to 56-char hex string."""
    return raw.hex()


def hash_with_salt(data: bytes, salt: int) -> bytes:
    """Replicate Common::Hasher::hashWithSalt — SHA3-224(data + salt_le_8bytes)."""
    salt_bytes = salt.to_bytes(8, byteorder="little")
    h = hashlib.sha3_224()
    h.update(data)
    h.update(salt_bytes)
    return h.digest()


def pack_header(msg_type: int, body: bytes, sender_id: bytes = b"\x00" * HASH_SIZE) -> bytes:
    """Build a 36-byte wire header + body.  Big-endian u32 for type and size."""
    return struct.pack(">II", msg_type, len(body)) + sender_id + body


async def read_message(reader: asyncio.StreamReader) -> tuple[int, bytes, bytes]:
    """Read one framed message from the Core.  Returns (msg_type, sender_id, body)."""
    header = await reader.readexactly(HEADER_SIZE)
    msg_type, body_size = struct.unpack(">II", header[:8])
    sender_id = header[8:]
    body = await reader.readexactly(body_size) if body_size else b""
    return msg_type, sender_id, body


class CoreClient:
    """Async TCP client that speaks the DG-LAN GUI protocol to a local Core."""

    def __init__(self, host: str, port: int, password: str = ""):
        self.host = host
        self.port = port
        self.password = password
        self.reader: Optional[asyncio.StreamReader] = None
        self.writer: Optional[asyncio.StreamWriter] = None
        self.peer_id: bytes = b""           # 28-byte local peer ID
        self.peer_hex: str = ""             # 56-char hex version
        self.shared_entries: list[dict] = []
        self._state: Optional[gui.State] = None
        self._browse_futures: dict[int, asyncio.Future] = {}  # tag → Future[BrowseResult]
        self._pending_browse_future: Optional[asyncio.Future] = None  # awaiting tag assignment
        self._listener_task: Optional[asyncio.Task] = None
        self._connected = False

    # ── Connection / auth ─────────────────────────────────────
    async def connect(self):
        log.info("Connecting to Core at %s:%d ...", self.host, self.port)
        self.reader, self.writer = await asyncio.open_connection(self.host, self.port)

        # Step 1: Core sends AskForAuthentication
        msg_type, _, body = await read_message(self.reader)
        if msg_type != GUI_ASK_FOR_AUTHENTICATION:
            raise RuntimeError(f"Expected AskForAuthentication, got 0x{msg_type:04x}")

        ask = gui.AskForAuthentication()
        ask.ParseFromString(body)
        log.info("Auth challenge received (salt=%d, salt_challenge=%d)", ask.salt, ask.salt_challenge)

        # Step 2: Compute password challenge
        if self.password:
            pw_hash = hash_with_salt(self.password.encode("utf-8"), ask.salt)
        else:
            pw_hash = b"\x00" * HASH_SIZE

        challenge = hash_with_salt(pw_hash, ask.salt_challenge)

        auth = gui.Authentication()
        auth.password_challenge.hash = challenge
        auth_body = auth.SerializeToString()
        self.writer.write(pack_header(GUI_AUTHENTICATION, auth_body))
        await self.writer.drain()

        # Step 3: Read AuthenticationResult
        msg_type, _, body = await read_message(self.reader)
        if msg_type != GUI_AUTHENTICATION_RESULT:
            raise RuntimeError(f"Expected AuthenticationResult, got 0x{msg_type:04x}")

        result = gui.AuthenticationResult()
        result.ParseFromString(body)
        if result.status != gui.AuthenticationResult.AUTH_OK:
            status_name = gui.AuthenticationResult.Status.Name(result.status)
            raise RuntimeError(f"Authentication failed: {status_name}")

        log.info("Authenticated successfully")
        self._connected = True

        # Start background listener for async messages from Core
        self._listener_task = asyncio.create_task(self._listen())

    async def disconnect(self):
        self._connected = False
        if self._listener_task:
            self._listener_task.cancel()
            try:
                await self._listener_task
            except asyncio.CancelledError:
                pass
        if self.writer:
            self.writer.close()
            try:
                await self.writer.wait_closed()
            except Exception:
                pass

    # ── Background message listener ──────────────────────────
    async def _listen(self):
        try:
            while self._connected:
                msg_type, sender_id, body = await read_message(self.reader)
                await self._handle_message(msg_type, sender_id, body)
        except asyncio.CancelledError:
            raise
        except asyncio.IncompleteReadError:
            log.warning("Core closed connection")
            self._connected = False
        except Exception:
            log.exception("Listener error")
            self._connected = False

    async def _handle_message(self, msg_type: int, sender_id: bytes, body: bytes):
        if msg_type == GUI_STATE:
            state = gui.State()
            state.ParseFromString(body)
            self._state = state

            # First peer is always ourselves
            if state.peer:
                self.peer_id = state.peer[0].peer_id.hash
                self.peer_hex = hash_to_hex(self.peer_id)

            # Track shared entries
            self.shared_entries = []
            for se in state.shared_entry:
                self.shared_entries.append({
                    "id": hash_to_hex(se.entry.id.hash),
                    "name": se.entry.shared_name,
                    "path": se.entry.path,
                    "size": se.size,
                    "free_space": se.free_space,
                })

            # Acknowledge state so Core keeps sending updates
            self.writer.write(pack_header(GUI_STATE_RESULT, b""))
            await self.writer.drain()

        elif msg_type == GUI_BROWSE_TAG:
            tag_msg = gui.Tag()
            tag_msg.ParseFromString(body)
            tag = tag_msg.tag
            # Move the pending future to be keyed by the tag Core assigned
            if self._pending_browse_future is not None:
                self._browse_futures[tag] = self._pending_browse_future
                self._pending_browse_future = None
            log.debug("Browse tag: %d", tag)

        elif msg_type == GUI_BROWSE_RESULT:
            result = gui.BrowseResult()
            result.ParseFromString(body)
            tag = result.tag
            fut = self._browse_futures.pop(tag, None)
            if fut and not fut.done():
                fut.set_result(result)
            log.debug("Browse result for tag %d: %d entry groups", tag, len(result.entries))

        else:
            log.debug("Ignoring message type 0x%04x (%d bytes)", msg_type, len(body))

    # ── Wait for initial state ────────────────────────────────
    async def wait_for_state(self, timeout: float = 10.0):
        deadline = time.monotonic() + timeout
        while self._state is None and time.monotonic() < deadline:
            await asyncio.sleep(0.1)
        if self._state is None:
            raise TimeoutError("No GUI_STATE received from Core")
        log.info("Got state: peer_id=%s, %d shared entries", self.peer_hex, len(self.shared_entries))

    # ── Browse files ──────────────────────────────────────────
    async def browse(self, shared_entry_hash: bytes = b"", path: str = "", timeout: float = 15.0) -> gui.BrowseResult:
        """Send a Browse request and wait for the result."""
        browse = gui.Browse()
        browse.peer_id.hash = self.peer_id
        browse.get_roots = True

        if shared_entry_hash:
            entry = browse.dirs.entry.add()
            entry.type = common.Entry.DIR
            entry.path = path or "/"
            entry.shared_entry.id.hash = shared_entry_hash

        # Create a future BEFORE sending so we don't miss the tag/result
        loop = asyncio.get_event_loop()
        fut: asyncio.Future[gui.BrowseResult] = loop.create_future()
        self._pending_browse_future = fut

        body = browse.SerializeToString()
        self.writer.write(pack_header(GUI_BROWSE, body))
        await self.writer.drain()

        return await asyncio.wait_for(fut, timeout=timeout)

    # ── Recursive file walk ───────────────────────────────────
    async def get_all_files(self) -> list[dict]:
        """Walk all shared directories and return all files with dglan:// link params."""
        if not self._state:
            await self.wait_for_state()

        all_files = []

        # First browse: get roots
        result = await self.browse()

        for entries_group in result.entries:
            for entry in entries_group.entry:
                await self._collect_files(entry, all_files)

        return all_files

    async def _collect_files(self, entry, all_files: list, depth: int = 0):
        if depth > 20:  # safety limit
            return

        if entry.type == common.Entry.FILE:
            # Build the file record with everything needed for a dglan:// link
            se_hash = hash_to_hex(entry.shared_entry.id.hash) if entry.shared_entry.id.hash else ""
            chunks = [hash_to_hex(c.hash) for c in entry.chunk if c.hash]

            all_files.append({
                "name": entry.name,
                "path": entry.path or "/",
                "size": entry.size,
                "shared_entry_hash": se_hash,
                "peer": self.peer_hex,
                "chunks": chunks,
                "dglan_url": self._build_dglan_url(entry),
            })

        elif entry.type == common.Entry.DIR and entry.shared_entry.id.hash:
            # Recurse into subdirectories
            try:
                sub_result = await self.browse(
                    shared_entry_hash=entry.shared_entry.id.hash,
                    path=entry.path + entry.name + "/" if entry.path else "/" + entry.name + "/",
                )
                for entries_group in sub_result.entries:
                    for sub_entry in entries_group.entry:
                        await self._collect_files(sub_entry, all_files, depth + 1)
            except TimeoutError:
                log.warning("Timeout browsing dir: %s/%s", entry.path, entry.name)

    def _build_dglan_url(self, entry) -> str:
        se_hash = hash_to_hex(entry.shared_entry.id.hash) if entry.shared_entry.id.hash else ""
        params = {
            "peer": self.peer_hex,
            "hash": se_hash,
            "size": str(entry.size),
            "name": entry.name,
            "path": entry.path or "/",
        }
        return "dglan://download?" + urllib.parse.urlencode(params, quote_via=urllib.parse.quote)


# ── HTTP Server ───────────────────────────────────────────────

class APIServer:
    """Minimal async HTTP server serving DG-LAN file data as JSON."""

    def __init__(self, core_client: CoreClient, http_host: str, http_port: int,
                 allowed_origins: list[str]):
        self.core = core_client
        self.http_host = http_host
        self.http_port = http_port
        self.allowed_origins = allowed_origins
        self._cache: Optional[dict] = None
        self._cache_time: float = 0
        self._cache_ttl: float = 30.0  # seconds

    async def start(self):
        server = await asyncio.start_server(self._handle_request, self.http_host, self.http_port)
        addrs = ", ".join(str(s.getsockname()) for s in server.sockets)
        log.info("HTTP API listening on %s", addrs)
        async with server:
            await server.serve_forever()

    async def _handle_request(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        try:
            request_line = await asyncio.wait_for(reader.readline(), timeout=10.0)
            if not request_line:
                writer.close()
                return

            request_str = request_line.decode("utf-8", errors="replace").strip()
            parts = request_str.split()
            if len(parts) < 2:
                writer.close()
                return

            method, path = parts[0], parts[1]

            # Read headers
            headers = {}
            while True:
                line = await asyncio.wait_for(reader.readline(), timeout=5.0)
                line_str = line.decode("utf-8", errors="replace").strip()
                if not line_str:
                    break
                if ":" in line_str:
                    key, val = line_str.split(":", 1)
                    headers[key.strip().lower()] = val.strip()

            origin = headers.get("origin", "")
            cors_headers = self._cors_headers(origin)

            if method == "OPTIONS":
                await self._send_response(writer, HTTPStatus.NO_CONTENT, b"", cors_headers)
                return

            if method != "GET":
                await self._send_json_response(writer, {"error": "Method not allowed"}, HTTPStatus.METHOD_NOT_ALLOWED, cors_headers)
                return

            # Route
            url_path = path.split("?")[0]

            if url_path == "/api/files":
                data = await self._get_files()
                await self._send_json_response(writer, data, HTTPStatus.OK, cors_headers)

            elif url_path == "/api/status":
                data = self._get_status()
                await self._send_json_response(writer, data, HTTPStatus.OK, cors_headers)

            elif url_path == "/api/health":
                await self._send_json_response(writer, {"status": "ok"}, HTTPStatus.OK, cors_headers)

            else:
                await self._send_json_response(writer, {"error": "Not found"}, HTTPStatus.NOT_FOUND, cors_headers)

        except Exception:
            log.exception("Request handling error")
            try:
                await self._send_json_response(writer, {"error": "Internal server error"}, HTTPStatus.INTERNAL_SERVER_ERROR, {})
            except Exception:
                pass
        finally:
            try:
                writer.close()
                await writer.wait_closed()
            except Exception:
                pass

    async def _get_files(self) -> dict:
        now = time.monotonic()
        if self._cache and (now - self._cache_time) < self._cache_ttl:
            return self._cache

        if not self.core._connected:
            return {"error": "Not connected to Core", "peer": "", "files": []}

        try:
            files = await self.core.get_all_files()
            data = {
                "peer": self.core.peer_hex,
                "shared_entries": self.core.shared_entries,
                "files": files,
                "timestamp": int(time.time()),
            }
            self._cache = data
            self._cache_time = now
            return data
        except Exception as e:
            log.exception("Error fetching files")
            return {"error": str(e), "peer": self.core.peer_hex, "files": []}

    def _get_status(self) -> dict:
        state = self.core._state
        if not state:
            return {"connected": self.core._connected, "peer": "", "cache_status": "unknown"}

        return {
            "connected": self.core._connected,
            "peer": self.core.peer_hex,
            "cache_status": gui.State.Stats.CacheStatus.Name(state.stats.cache_status),
            "cache_progress": state.stats.progress / 100.0,
            "shared_entries": len(self.core.shared_entries),
            "peers_online": len(state.peer),
        }

    def _cors_headers(self, origin: str) -> dict[str, str]:
        if not self.allowed_origins or "*" in self.allowed_origins:
            allowed = "*"
        elif origin in self.allowed_origins:
            allowed = origin
        else:
            allowed = ""

        if not allowed:
            return {}

        return {
            "Access-Control-Allow-Origin": allowed,
            "Access-Control-Allow-Methods": "GET, OPTIONS",
            "Access-Control-Allow-Headers": "Content-Type",
            "Access-Control-Max-Age": "86400",
        }

    async def _send_json_response(self, writer: asyncio.StreamWriter, data: dict,
                                   status: HTTPStatus, extra_headers: dict):
        body = json.dumps(data, ensure_ascii=False, indent=2).encode("utf-8")
        headers = {"Content-Type": "application/json; charset=utf-8"}
        headers.update(extra_headers)
        await self._send_response(writer, status, body, headers)

    async def _send_response(self, writer: asyncio.StreamWriter, status: HTTPStatus,
                              body: bytes, headers: dict):
        status_line = f"HTTP/1.1 {status.value} {status.phrase}\r\n"
        header_lines = "".join(f"{k}: {v}\r\n" for k, v in headers.items())
        header_lines += f"Content-Length: {len(body)}\r\n"
        header_lines += "Connection: close\r\n"
        writer.write((status_line + header_lines + "\r\n").encode("utf-8"))
        if body:
            writer.write(body)
        await writer.drain()


# ── Main ──────────────────────────────────────────────────────

async def main(args):
    core = CoreClient(args.core_host, args.core_port, args.password or "")
    api = APIServer(core, args.http_host, args.http_port, args.cors_origins)

    while True:
        try:
            await core.connect()
            await core.wait_for_state()
            log.info("Core connected. Peer ID: %s", core.peer_hex)
            await api.start()
        except KeyboardInterrupt:
            break
        except Exception:
            log.exception("Connection failed, retrying in 5s...")
            await core.disconnect()
            await asyncio.sleep(5)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="DG-LAN HTTP API Server")
    parser.add_argument("--core-host", default="127.0.0.1", help="Core daemon IP (default: 127.0.0.1)")
    parser.add_argument("--core-port", type=int, default=59485, help="Core remote control port (default: 59485)")
    parser.add_argument("--password", default="", help="Core remote password (empty = no auth for local)")
    parser.add_argument("--http-host", default="0.0.0.0", help="HTTP listen address (default: 0.0.0.0)")
    parser.add_argument("--http-port", type=int, default=8080, help="HTTP listen port (default: 8080)")
    parser.add_argument("--cors-origins", nargs="*", default=["*"], help="Allowed CORS origins (default: *)")
    parser.add_argument("--verbose", "-v", action="store_true", help="Debug logging")
    args = parser.parse_args()

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s %(levelname)-5s %(name)s  %(message)s",
    )

    asyncio.run(main(args))
