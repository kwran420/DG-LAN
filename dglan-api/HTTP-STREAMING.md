# DG-LAN HTTP File Streaming & Load Balancing

Detailed documentation for the HTTP streaming endpoint in the DG-LAN API server.
This feature lets users click a standard HTTP link on a website and receive a file
that was load-balanced across multiple LAN peers by DG-LAN Core.

---

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [How Load Balancing Works](#how-load-balancing-works)
- [Endpoint Reference](#endpoint-reference)
  - [Stream a file](#stream-a-file)
  - [File listing with HTTP URLs](#file-listing-with-http-urls)
- [HTTP Headers](#http-headers)
  - [Range Requests (Resume Support)](#range-requests-resume-support)
  - [ETag Conditional Requests (Caching)](#etag-conditional-requests-caching)
  - [Content-Type Detection](#content-type-detection)
  - [Content-Disposition (Inline vs Download)](#content-disposition-inline-vs-download)
  - [CORS](#cors)
- [JavaScript Integration](#javascript-integration)
  - [Basic Setup](#basic-setup)
  - [Building Stream URLs](#building-stream-urls)
  - [Complete Website Example](#complete-website-example)
- [URL Structure](#url-structure)
- [Error Responses](#error-responses)
- [Security](#security)
- [Configuration & Deployment](#configuration--deployment)
  - [Command-Line Options](#command-line-options)
  - [Reverse Proxy (Production)](#reverse-proxy-production)
  - [Binding to All Interfaces](#binding-to-all-interfaces)
- [Troubleshooting](#troubleshooting)
- [Testing](#testing)
  - [Manual Testing](#manual-testing)
  - [Automated Tests](#automated-tests)

---

## Architecture Overview

```
┌───────────────┐         ┌────────────┐         ┌────────────┐
│  LAN Peers    │  chunks  │  DG-LAN    │  TCP     │  dglan-api │  HTTP
│  (3+ machines)│────────►│  Core      │◄────────►│  server.py │◄────────► Browser
│               │  64 MiB  │  (daemon)  │ protobuf │  (Python)  │  stream
└───────────────┘  each    └────────────┘  :59485  └────────────┘  :8080

                  ▲                          ▲                        ▲
            Load balancing           Shared file index          File streaming
           happens HERE              maintained HERE            served HERE
```

**Data flow:**

1. **DG-LAN Core** discovers peers on the LAN and downloads files by requesting
   chunks (64 MiB each) from multiple peers simultaneously — this is the
   load balancing layer.
2. **dglan-api** (`server.py`) connects to the local Core over TCP protobuf,
   receives the shared file index (hashes, paths, sizes), and maintains a
   lookup table mapping shared entry hashes to filesystem paths.
3. **The browser** requests a file via the HTTP streaming endpoint. The API
   server resolves the shared entry hash and relative path to an absolute
   file on disk, then streams it back with proper HTTP semantics.

The key insight: **by the time a file is streamable, the load balancing has
already happened**. Core assembled the file from chunks pulled in parallel
from every available peer. The HTTP endpoint simply serves the finished result.

---

## How Load Balancing Works

DG-LAN's load balancing is **automatic and chunk-level**. Here's what happens
when a file becomes available:

### Chunk-Level Parallelism

```
File: game-installer.exe (256 MiB)
      = 4 chunks × 64 MiB each

Peer A ──► Chunk 1 ──┐
Peer B ──► Chunk 2 ──┤
Peer C ──► Chunk 3 ──├──► Complete file on disk
Peer A ──► Chunk 4 ──┘
           (A was fastest, got a second chunk)
```

- Files are split into **64 MiB chunks**, each identified by a SHA3-224 hash.
- Core requests different chunks from different peers simultaneously.
- Faster peers naturally serve more chunks (adaptive balancing).
- If a peer goes offline mid-transfer, remaining chunks are reassigned.
- All of this is transparent — the HTTP endpoint just serves the assembled file.

### What This Means for Your Website

- Users click a normal HTTP link → file streams from local disk at full speed.
- The file was already load-balanced during download — no runtime overhead.
- Multiple users can stream the same file simultaneously (standard OS file I/O).
- No peer connections or protocol knowledge needed on the browser side.

---

## Endpoint Reference

### Stream a file

```
GET /api/v1/files/{shared_entry_hash}/{path}
```

Streams a file directly over HTTP. Returns the raw file bytes with appropriate
headers for browser playback, display, or download.

| Parameter | Location | Description |
|-----------|----------|-------------|
| `shared_entry_hash` | URL path | 56-character hex SHA3-224 hash identifying the shared directory |
| `path` | URL path | Relative path to the file within the shared directory (URL-encoded) |
| `download` | Query string | Set to `1` to force a download dialog instead of inline display |

**Request:**
```http
GET /api/v1/files/a1b2c3d4e5f6...9z/Videos/Cats/lolcat.mp4 HTTP/1.1
Host: 192.168.1.100:8080
```

**Response (200 OK):**
```http
HTTP/1.1 200 OK
Content-Type: video/mp4
Content-Length: 15728640
Content-Disposition: inline; filename="lolcat.mp4"
Accept-Ranges: bytes
ETag: "a1b2c3d4e5f6...:1712345678:15728640"
Cache-Control: private, max-age=60
Connection: close

<raw file bytes>
```

**Force download:**
```http
GET /api/v1/files/a1b2c3d4e5f6...9z/Videos/Cats/lolcat.mp4?download=1 HTTP/1.1
```
```http
HTTP/1.1 200 OK
Content-Disposition: attachment; filename="lolcat.mp4"
...
```

### File listing with HTTP URLs

```
GET /api/v1/files
```

Returns JSON with all shared files. Each file object now includes an `http_url`
field containing the pre-built streaming path:

```json
{
  "peer": "0011aabbccdd...",
  "shared_entries": [
    {
      "id": "aabb1122...",
      "name": "Games",
      "path": "C:/Shared/Games/",
      "size": 52428800,
      "free_space": 107374182400
    }
  ],
  "files": [
    {
      "name": "movie.mkv",
      "path": "/",
      "size": 1073741824,
      "shared_entry_hash": "aabb1122...",
      "peer": "0011aabbccdd...",
      "chunks": ["ff00aa...", "bb11cc..."],
      "http_url": "/api/v1/files/aabb1122.../movie.mkv",
      "dglan_url": "dglan://download?peer=0011aa...&hash=aabb11...&size=1073741824&name=movie.mkv&path=/"
    }
  ],
  "timestamp": 1712345678
}
```

The `http_url` is a relative path. Prepend your server's base URL to create a
full link:

```
http://192.168.1.100:8080/api/v1/files/aabb1122.../movie.mkv
```

---

## HTTP Headers

### Range Requests (Resume Support)

The streaming endpoint fully supports RFC 7233 Range requests, enabling:

- **Resume interrupted downloads** (browser or download manager sends
  `Range: bytes=1048576-` to continue from byte 1 MiB)
- **Seek within video/audio** (media players use Range to jump to a timestamp)
- **Partial content delivery** (fetch only a slice of a large file)

**Example — first 1 KiB:**
```http
GET /api/v1/files/aabb.../movie.mkv HTTP/1.1
Range: bytes=0-1023
```
```http
HTTP/1.1 206 Partial Content
Content-Range: bytes 0-1023/1073741824
Content-Length: 1024
Accept-Ranges: bytes
...
```

**Example — last 100 bytes:**
```http
Range: bytes=-100
```
```http
HTTP/1.1 206 Partial Content
Content-Range: bytes 1073741724-1073741823/1073741824
Content-Length: 100
```

**Example — from byte 500 to end:**
```http
Range: bytes=500-
```
```http
HTTP/1.1 206 Partial Content
Content-Range: bytes 500-1073741823/1073741824
Content-Length: 1073741324
```

**Invalid ranges** return `416 Range Not Satisfiable`:
```http
HTTP/1.1 416 Range Not Satisfiable
Content-Range: bytes */1073741824
```

Range behavior:
- Only single-range requests are supported (no multi-part ranges).
- End positions beyond file size are clamped to the last byte (per RFC 7233 §2.1).
- Start beyond file size returns 416.

### ETag Conditional Requests (Caching)

Every response includes an `ETag` header derived from the shared entry hash,
file modification time, and file size. Clients can use `If-None-Match` to
avoid re-downloading unchanged files.

**First request:**
```http
GET /api/v1/files/aabb.../movie.mkv HTTP/1.1
```
```http
HTTP/1.1 200 OK
ETag: "aabb112233445566:1712345678:1073741824"
...
<file bytes>
```

**Subsequent request (cache hit):**
```http
GET /api/v1/files/aabb.../movie.mkv HTTP/1.1
If-None-Match: "aabb112233445566:1712345678:1073741824"
```
```http
HTTP/1.1 304 Not Modified
ETag: "aabb112233445566:1712345678:1073741824"
```

The browser automatically handles this — no JavaScript needed. The `Cache-Control: private, max-age=60` header also enables short-term browser caching.

### Content-Type Detection

The server detects MIME types from file extensions using Python's `mimetypes`
module. Common mappings:

| Extension | Content-Type |
|-----------|-------------|
| `.mp4` | `video/mp4` |
| `.mkv` | `video/x-matroska` |
| `.avi` | `video/x-msvideo` |
| `.mp3` | `audio/mpeg` |
| `.flac` | `audio/flac` |
| `.jpg` / `.jpeg` | `image/jpeg` |
| `.png` | `image/png` |
| `.gif` | `image/gif` |
| `.pdf` | `application/pdf` |
| `.zip` | `application/zip` |
| `.exe` | `application/x-msdownload` |
| `.txt` | `text/plain` |
| *(unknown)* | `application/octet-stream` |

Browsers use this to decide whether to display the file inline (images, video,
PDF) or prompt for download (executables, archives).

### Content-Disposition (Inline vs Download)

By default, files are served with `Content-Disposition: inline`, letting the
browser display or play them directly. Add `?download=1` to force a save dialog:

| URL | Behavior | Header |
|-----|----------|--------|
| `/api/v1/files/aabb.../photo.jpg` | Displays in browser | `inline; filename="photo.jpg"` |
| `/api/v1/files/aabb.../photo.jpg?download=1` | Opens save dialog | `attachment; filename="photo.jpg"` |
| `/api/v1/files/aabb.../game.zip` | Browser decides (usually downloads) | `inline; filename="game.zip"` |
| `/api/v1/files/aabb.../game.zip?download=1` | Forces save dialog | `attachment; filename="game.zip"` |

### CORS

If `--cors-origins` is configured, CORS headers are included in streaming
responses, allowing cross-origin requests from your website:

```bash
python server.py --cors-origins https://mysite.com
```

Headers returned:
```http
Access-Control-Allow-Origin: https://mysite.com
Access-Control-Allow-Methods: GET, OPTIONS
Access-Control-Allow-Headers: Content-Type
Access-Control-Max-Age: 86400
```

---

## JavaScript Integration

### Basic Setup

Include `dglan-api.js` in your HTML:

```html
<script src="dglan-api.js"></script>
<script>
  const api = new DglanApi("http://192.168.1.100:8080");
</script>
```

Or in Node.js:

```javascript
const DglanApi = require("./dglan-api.js");
const api = new DglanApi("http://192.168.1.100:8080");
```

### Building Stream URLs

Use `buildStreamUrl()` to construct HTTP streaming URLs from file data:

```javascript
const data = await api.getFiles();

for (const file of data.files) {
  // Inline/playback URL (opens in browser)
  const url = api.buildStreamUrl(file);
  // → "http://192.168.1.100:8080/api/v1/files/aabb.../movie.mkv"

  // Forced download URL
  const downloadUrl = api.buildStreamUrl({ ...file, download: true });
  // → "http://192.168.1.100:8080/api/v1/files/aabb.../movie.mkv?download=1"
}
```

`buildStreamUrl()` accepts an object with these properties:

| Property | Required | Description |
|----------|----------|-------------|
| `shared_entry_hash` | Yes | 56-char hex shared entry identifier |
| `name` | Yes | Filename |
| `path` | No | Directory path within the shared entry (defaults to root) |
| `download` | No | `true` to append `?download=1` |

The method also handles URL encoding automatically, so filenames with spaces
or special characters work correctly.

### Pre-Built URLs

The file listing JSON already includes a pre-built `http_url` for convenience:

```javascript
const data = await api.getFiles();
for (const file of data.files) {
  // Use the pre-built relative URL directly
  const fullUrl = `${api.baseUrl}${file.http_url}`;
}
```

### Complete Website Example

```html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>File Server</title>
  <style>
    body { font-family: system-ui, sans-serif; max-width: 900px; margin: 2rem auto; }
    table { width: 100%; border-collapse: collapse; }
    th, td { padding: 0.5rem; text-align: left; border-bottom: 1px solid #ddd; }
    a { color: #0066cc; text-decoration: none; }
    a:hover { text-decoration: underline; }
    .size { color: #666; font-variant-numeric: tabular-nums; }
    .download { margin-left: 0.5rem; font-size: 0.85em; }
  </style>
</head>
<body>
  <h1>Shared Files</h1>
  <table>
    <thead><tr><th>Name</th><th>Size</th><th>Path</th></tr></thead>
    <tbody id="files"></tbody>
  </table>

  <script src="dglan-api.js"></script>
  <script>
    (async () => {
      const api = new DglanApi(location.origin);
      const data = await api.getFiles();
      const tbody = document.getElementById("files");

      for (const file of data.files) {
        const tr = document.createElement("tr");

        // File name — clickable link that streams/plays in browser
        const tdName = document.createElement("td");
        const link = document.createElement("a");
        link.href = api.buildStreamUrl(file);
        link.textContent = file.name;
        tdName.appendChild(link);

        // Download button
        const dl = document.createElement("a");
        dl.href = api.buildStreamUrl({ ...file, download: true });
        dl.textContent = "⬇";
        dl.title = "Download";
        dl.className = "download";
        tdName.appendChild(dl);

        // File size
        const tdSize = document.createElement("td");
        tdSize.className = "size";
        tdSize.textContent = DglanApi.formatSize(file.size);

        // Path within shared directory
        const tdPath = document.createElement("td");
        tdPath.textContent = file.path;

        tr.append(tdName, tdSize, tdPath);
        tbody.appendChild(tr);
      }
    })();
  </script>
</body>
</html>
```

### Advanced: Video Player Integration

```html
<!-- Stream video directly — browser handles Range requests for seeking -->
<video controls width="720">
  <source src="http://192.168.1.100:8080/api/v1/files/aabb.../movie.mp4" type="video/mp4">
</video>
```

```html
<!-- Audio player -->
<audio controls>
  <source src="http://192.168.1.100:8080/api/v1/files/aabb.../song.mp3" type="audio/mpeg">
</audio>
```

Because the endpoint supports Range requests, the `<video>` and `<audio>`
elements can seek to any position without downloading the entire file first.

### Advanced: Fetch API with Range

```javascript
// Download bytes 0-999 of a file
const url = api.buildStreamUrl(file);
const resp = await fetch(url, {
  headers: { "Range": "bytes=0-999" }
});
console.log(resp.status);        // 206
console.log(resp.headers.get("Content-Range")); // "bytes 0-999/15728640"
const slice = await resp.arrayBuffer();
```

---

## URL Structure

```
/api/v1/files/{shared_entry_hash}/{relative_path}[?download=1]
 ─────┬──────  ────────┬────────  ──────┬───────   ─────┬─────
       │               │                │               │
  API version    56-char hex       Path inside       Optional
  prefix         SHA3-224 hash     shared dir        query param
                 of the shared
                 directory
```

**Examples:**

| URL | Description |
|-----|-------------|
| `/api/v1/files/aabb.../readme.txt` | File at root of shared directory |
| `/api/v1/files/aabb.../Games/doom.zip` | File in a subdirectory |
| `/api/v1/files/aabb.../My%20Files/doc.pdf` | Space in path (URL-encoded) |
| `/api/v1/files/aabb.../game.iso?download=1` | Force download dialog |

**Finding the shared entry hash:** Call `GET /api/v1/files` — each file's
`shared_entry_hash` field is the value to use. The `http_url` field already
contains the complete pre-built path.

---

## Error Responses

All errors follow a consistent JSON format:

```json
{
  "error": {
    "code": "ERROR_CODE",
    "message": "Human-readable description"
  }
}
```

### Error Codes

| HTTP Status | Error Code | Cause | Fix |
|-------------|-----------|-------|-----|
| 400 | `INVALID_PATH` | Missing or empty file path after the hash | Include a filename: `/api/v1/files/{hash}/filename.ext` |
| 400 | `INVALID_HASH` | shared_entry_hash is not 56 hex characters | Use exact hash from the file listing |
| 404 | `ENTRY_NOT_FOUND` | No shared directory matches that hash | Refresh your file listing (`GET /api/v1/files`) |
| 404 | `FILE_NOT_FOUND` | File doesn't exist at that path | Check spelling, path separators, and URL encoding |
| 405 | `METHOD_NOT_ALLOWED` | Used POST/PUT/DELETE instead of GET | Only GET and OPTIONS are supported |
| 416 | *(no JSON body)* | Range header requests bytes beyond file | Check `Content-Range: bytes */SIZE` in the response |
| 503 | `CORE_DISCONNECTED` | API server lost connection to DG-LAN Core | Ensure Core is running and restart the API server |
| 500 | `IO_ERROR` | Disk read failure while streaming | Check file permissions and disk health |
| 500 | `INTERNAL_ERROR` | Unexpected server error | Check server logs (`--verbose`) |

---

## Security

### Path Traversal Prevention

The server guards against directory traversal attacks. These requests are all
blocked and return `404 FILE_NOT_FOUND`:

```
/api/v1/files/aabb.../../../etc/passwd          → blocked
/api/v1/files/aabb.../..%2F..%2Fetc%2Fpasswd    → blocked (decoded then checked)
/api/v1/files/aabb.../valid/../../../secret      → blocked
/api/v1/files/aabb../C:%5CWindows%5Csystem32    → blocked (Windows path)
```

How it works:
1. The relative path is URL-decoded and joined with the shared directory root.
2. Both the root and joined path are resolved to their real absolute paths
   via `os.path.realpath()` (resolves symlinks, `..`, etc.).
3. `os.path.commonpath()` verifies the resolved path shares the same root.
4. Any path that escapes the shared directory is rejected.
5. Null bytes (`\x00`) in paths are rejected outright.

### Hash Validation

The shared entry hash must be exactly 56 lowercase hexadecimal characters
(SHA3-224). Any other format is rejected with `400 INVALID_HASH`.

### Header Injection Prevention

Filenames in `Content-Disposition` headers are sanitized: double quotes,
carriage returns, and newlines are stripped to prevent HTTP response splitting.

### Recommendation: Run Behind a Reverse Proxy

For production deployments, always run behind nginx or Caddy with HTTPS:

```
Internet → nginx (HTTPS, :443) → dglan-api (HTTP, 127.0.0.1:8080)
```

This provides TLS encryption, rate limiting, and additional security headers.

---

## Configuration & Deployment

### Command-Line Options

```bash
python server.py [options]
```

| Flag | Default | Description |
|------|---------|-------------|
| `--core-host` | `127.0.0.1` | DG-LAN Core daemon IP address |
| `--core-port` | `59485` | Core remote control port |
| `--password` | *(empty)* | Core password (prefer `DGLAN_PASSWORD` env var) |
| `--http-host` | `127.0.0.1` | HTTP listen address |
| `--http-port` | `8080` | HTTP listen port |
| `--cors-origins` | *(none)* | Allowed CORS origins (space-separated) |
| `--verbose` | off | Enable debug logging |

### Binding to All Interfaces

By default, the server only listens on localhost. To allow other machines
on your network to access the streaming endpoint:

```bash
python server.py --http-host 0.0.0.0
```

> **Warning:** This exposes all shared files to anyone on your network.
> Use `--cors-origins` and a reverse proxy to control access.

### Reverse Proxy (Production)

**nginx:**
```nginx
server {
    listen 443 ssl;
    server_name files.example.com;

    ssl_certificate     /etc/ssl/cert.pem;
    ssl_certificate_key /etc/ssl/key.pem;

    # Streaming endpoint — proxy with buffering disabled for large files
    location /api/v1/files/ {
        proxy_pass http://127.0.0.1:8080;
        proxy_buffering off;
        proxy_request_buffering off;
        proxy_http_version 1.1;
        client_max_body_size 0;
    }

    # JSON endpoints
    location /api/ {
        proxy_pass http://127.0.0.1:8080;
    }
}
```

Key nginx settings for streaming:
- `proxy_buffering off` — don't buffer the entire file in nginx memory
- `client_max_body_size 0` — no upload size limit (for future POST endpoints)
- `proxy_http_version 1.1` — enables keep-alive to the backend

**Caddy:**
```
files.example.com {
    reverse_proxy /api/* 127.0.0.1:8080 {
        flush_interval -1
    }
}
```

### Systemd Service (Linux)

```ini
[Unit]
Description=DG-LAN API Server
After=network.target

[Service]
Type=simple
User=dglan
WorkingDirectory=/opt/dglan-api
ExecStart=/usr/bin/python3 server.py --http-host 0.0.0.0 --cors-origins https://files.example.com
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

### Windows Task Scheduler

Create a scheduled task to run at startup:

```powershell
$Action = New-ScheduledTaskAction -Execute "python" -Argument "server.py --http-host 0.0.0.0" -WorkingDirectory "C:\DG-LAN\dglan-api"
$Trigger = New-ScheduledTaskTrigger -AtStartup
Register-ScheduledTask -TaskName "DG-LAN API" -Action $Action -Trigger $Trigger -RunLevel Highest
```

---

## Troubleshooting

### File returns 404 but I can see it in the file listing

1. **Path encoding:** Spaces and special characters must be URL-encoded.
   `My File.txt` → `My%20File.txt`
2. **Case sensitivity:** Windows paths are case-insensitive, but the URL must
   match what the file listing returns.
3. **Stale index:** The file index updates when Core sends state updates. If
   you just added a shared directory, wait a few seconds and refresh.

### Video won't seek / shows spinner

Ensure your reverse proxy isn't stripping the `Range` header or buffering the
response. In nginx, add `proxy_buffering off;` to the streaming location block.

### "CORE_DISCONNECTED" error

The API server lost its TCP connection to the DG-LAN Core daemon.

1. Is Core running? Check the DG-LAN GUI or `DG-LAN.Core.exe` process.
2. Restart the API server — it will reconnect automatically.
3. If using `--core-host` for a remote Core, verify network connectivity.

### "ENTRY_NOT_FOUND" error

The shared entry hash in your URL doesn't match any known shared directory.

1. Call `GET /api/v1/files` to get the current file listing.
2. Use the `shared_entry_hash` value from the response.
3. If you recently changed shared directories in the GUI, wait for the index to update.

### Large files are slow

The server streams files in 64 KiB chunks, which is efficient for most cases.
If you're serving very large files (10+ GB) to many concurrent users, consider:

1. Putting nginx in front with `sendfile on;` and `tcp_nopush on;`
2. Using a CDN for public-facing deployments
3. Ensuring the disk can sustain the required I/O throughput

### CORS errors in browser console

```
Access to fetch at 'http://...' from origin 'https://...' has been blocked by CORS policy
```

Start the server with the correct origin:

```bash
python server.py --cors-origins https://your-website.com
```

For development, you can allow all origins:

```bash
python server.py --cors-origins "*"
```

---

## Testing

### Manual Testing

**1. Start the server:**
```bash
cd dglan-api
python server.py --verbose
```

**2. Get the file listing:**
```bash
curl http://127.0.0.1:8080/api/v1/files | python -m json.tool
```

**3. Stream a file (use an `http_url` from the listing):**
```bash
curl -o output.mkv http://127.0.0.1:8080/api/v1/files/aabb.../movie.mkv
```

**4. Test Range request:**
```bash
curl -v -H "Range: bytes=0-1023" http://127.0.0.1:8080/api/v1/files/aabb.../movie.mkv
# Should return: HTTP/1.1 206 Partial Content
```

**5. Test conditional request:**
```bash
# First request — get the ETag
ETAG=$(curl -sI http://127.0.0.1:8080/api/v1/files/aabb.../movie.mkv | grep -i etag | tr -d '\r' | cut -d' ' -f2)

# Second request — should return 304
curl -v -H "If-None-Match: $ETAG" http://127.0.0.1:8080/api/v1/files/aabb.../movie.mkv
# Should return: HTTP/1.1 304 Not Modified
```

**6. Test forced download:**
```bash
curl -v "http://127.0.0.1:8080/api/v1/files/aabb.../movie.mkv?download=1"
# Content-Disposition should say "attachment" instead of "inline"
```

**7. Open in browser:**
Paste any `http_url` (with the server base URL prepended) into your browser's
address bar. Video and audio files should play inline. Images should display.
Other files will either display or prompt for download depending on the browser.

### Automated Tests

The streamer has 59 unit tests covering all code paths. No running Core required.

```bash
cd dglan-api
pip install pytest
pytest test_streamer.py -v
```

**Test coverage:**

| Test Class | Count | What It Tests |
|------------|-------|---------------|
| `TestHashPattern` | 7 | SHA3-224 hash format validation |
| `TestResolve` | 12 | Path resolution, traversal prevention, edge cases |
| `TestParseRange` | 14 | RFC 7233 Range header parsing, suffix ranges, clamping |
| `TestComputeEtag` | 3 | ETag generation from file metadata |
| `TestGuessContentType` | 6 | MIME type detection for common file types |
| `TestStream` | 13 | Full streaming: 200, 206, 304, 416, Content-Disposition |
| `TestHTTPRouting` | 4 | URL routing: valid paths, missing paths, bad hashes |

All tests use temporary files and mock objects — they run in under a second and
don't touch the network or require a DG-LAN Core instance.
