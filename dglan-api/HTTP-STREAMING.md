# DG-LAN Python API Bridge — HTTP Streaming & `dglan://` Links

The `dglan-api/` Python server bridges a running DG-LAN Core to HTTP, providing:

1. **`dglan://` download links** — generate custom URL-scheme links for your website so visitors can download files directly into their DG-LAN client.
2. **HTTP file streaming** — stream files directly to browsers with Range support, ETag caching, and configurable CORS.
3. **JSON API** — query the file index, peer status, and health over HTTP.

> **Not sure which HTTP server to use?** DG-LAN also has a [built-in HTTP server](../HTTP-SERVER.md)
> compiled directly into Core (port 59480, zero dependencies). Use this Python bridge when you need
> `dglan://` links, configurable CORS, forced downloads, or a standalone deployment.
> See the [comparison table](../HTTP-SERVER.md#comparison-built-in-server-vs-python-bridge).

---

## Table of Contents

- [Architecture](#architecture)
- [Quick Start](#quick-start)
- [Core Concepts](#core-concepts)
  - [How the Bridge Connects to Core](#how-the-bridge-connects-to-core)
  - [How Load Balancing Works](#how-load-balancing-works)
  - [Shared Entry Hashes](#shared-entry-hashes)
- [API Reference](#api-reference)
  - [File Listing — GET /api/v1/files](#file-listing)
  - [File Streaming — GET /api/v1/files/{hash}/{path}](#file-streaming)
  - [Status — GET /api/v1/status](#status)
  - [Health Check — GET /api/v1/health](#health-check)
- [HTTP Features](#http-features)
  - [Range Requests (Resume & Seek)](#range-requests)
  - [ETag Caching](#etag-caching)
  - [Content-Type Detection](#content-type-detection)
  - [Content-Disposition (Inline vs Download)](#content-disposition)
  - [CORS](#cors)
- [JavaScript Integration](#javascript-integration)
  - [Basic Setup](#basic-setup)
  - [Building Stream URLs](#building-stream-urls)
  - [Building dglan:// Links](#building-dglan-links)
  - [Complete Website Example](#complete-website-example)
  - [Video and Audio Embedding](#video-and-audio-embedding)
- [URL Schemes](#url-schemes)
  - [HTTP Streaming URLs](#http-streaming-urls)
  - [dglan:// Download URLs](#dglan-download-urls)
- [Security](#security)
  - [Path Traversal Protection](#path-traversal-protection)
  - [Hash Validation](#hash-validation)
  - [Header Injection Prevention](#header-injection-prevention)
  - [Deployment Recommendations](#deployment-recommendations)
- [Configuration](#configuration)
  - [Command-Line Options](#command-line-options)
  - [Environment Variables](#environment-variables)
  - [Binding to All Interfaces](#binding-to-all-interfaces)
  - [Reverse Proxy (Production)](#reverse-proxy-production)
  - [Running as a Service](#running-as-a-service)
- [Error Reference](#error-reference)
- [Troubleshooting](#troubleshooting)
- [Testing](#testing)
  - [Manual Testing](#manual-testing)
  - [Automated Tests](#automated-tests)

---

## Architecture

```
┌───────────────┐         ┌────────────┐         ┌────────────┐
│  LAN Peers    │  chunks  │  DG-LAN    │  TCP     │  dglan-api │  HTTP
│  (machines)   │────────►│  Core      │◄────────►│  server.py │◄────────► Browser
│               │  64 MiB  │  (daemon)  │ protobuf │  (Python)  │  stream
└───────────────┘  each    └────────────┘  :59485  └────────────┘  :8080
```

**How data flows:**

1. **DG-LAN Core** discovers peers on the LAN and downloads files by pulling
   chunks (64 MiB each) from multiple peers simultaneously.
2. **`server.py`** connects to the local Core over TCP (protobuf, port 59485),
   authenticates, and receives the shared file index — hashes, paths, sizes.
3. **The browser** fetches `GET /api/v1/files` to list available files, then
   either streams a file over HTTP or launches a `dglan://` link for native download.

---

## Quick Start

```bash
# Install dependency
pip install protobuf

# Start (Core must be running on this machine)
cd dglan-api
python server.py

# With options
python server.py --http-host 0.0.0.0 --cors-origins https://mysite.com

# Remote Core with password (prefer environment variable)
DGLAN_PASSWORD=mysecret python server.py --core-host 10.0.0.5
```

The server starts on `http://127.0.0.1:8080` by default.

**Requirements:** Python 3.10+, `protobuf` package, DG-LAN Core running on the same or reachable machine.

---

## Core Concepts

### How the Bridge Connects to Core

The Python server speaks the same protobuf protocol as the DG-LAN GUI:

1. Connects via TCP to Core's remote control port (default 59485).
2. Core sends an authentication challenge.
3. Server responds with the password (SHA3-224 hashed). Local connections
   (127.0.0.1) are auto-accepted without a password.
4. Core pushes a `GUI_STATE` message containing the full peer list and shared file index.
5. The server caches this and rebuilds it periodically (30-second cache TTL).
6. When a file is requested, the server resolves the hash to a local filesystem path and streams the file.

### How Load Balancing Works

DG-LAN's load balancing is **chunk-level and automatic**:

```
File: game-installer.exe (256 MiB) = 4 chunks × 64 MiB

Peer A ──► Chunk 1 ──┐
Peer B ──► Chunk 2 ──┤
Peer C ──► Chunk 3 ──├──► Complete file on disk
Peer A ──► Chunk 4 ──┘
           (A was fastest, got a second chunk)
```

By the time a file is streamable through the HTTP endpoint, the load balancing
has already happened. Core assembled the file from chunks pulled from multiple
peers. The HTTP server simply serves the finished local copy at full disk speed.

### Shared Entry Hashes

Every shared directory in DG-LAN is identified by a **56-character lowercase hex string** — the SHA3-224 hash of the shared entry. This hash appears:

- In the file listing as `shared_entry_hash`
- In streaming URLs as the path component: `/api/v1/files/{shared_entry_hash}/...`
- In `dglan://` links as the `hash` parameter

You never need to construct these manually. Call the file listing endpoint and use the values it returns.

---

## API Reference

### File Listing

```
GET /api/v1/files
GET /api/files        (backward-compatible alias)
```

Returns all shared files with pre-built `dglan://` and HTTP streaming URLs.

**Response:**
```json
{
  "peer": "0011aabbccdd...56_hex_chars",
  "shared_entries": [
    {
      "id": "aabb1122...56_hex_chars",
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
      "shared_entry_hash": "aabb1122...56_hex_chars",
      "peer": "0011aabbccdd...56_hex_chars",
      "chunks": ["ff00aa...", "bb11cc..."],
      "http_url": "/api/v1/files/aabb1122.../movie.mkv",
      "dglan_url": "dglan://download?peer=0011aa...&hash=aabb11...&size=1073741824&name=movie.mkv&path=%2F"
    }
  ],
  "timestamp": 1712345678
}
```

| Field | Type | Description |
|-------|------|-------------|
| `peer` | string | This Core's 56-char hex peer ID |
| `shared_entries` | array | Shared directories registered in Core |
| `shared_entries[].id` | string | 56-char hex hash of the shared directory |
| `shared_entries[].name` | string | Directory name |
| `shared_entries[].path` | string | Absolute filesystem path |
| `shared_entries[].size` | integer | Used space in bytes |
| `shared_entries[].free_space` | integer | Free space on disk in bytes |
| `files` | array | All files in the shared index |
| `files[].name` | string | Filename |
| `files[].path` | string | Relative directory path within the shared entry (`/` for root) |
| `files[].size` | integer | File size in bytes |
| `files[].shared_entry_hash` | string | 56-char hex — use in `/api/v1/files/{hash}/{path}` |
| `files[].peer` | string | Peer ID that owns this file |
| `files[].chunks` | array | Chunk hashes (hex strings) |
| `files[].http_url` | string | Pre-built relative HTTP streaming URL |
| `files[].dglan_url` | string | Pre-built `dglan://` download link |
| `timestamp` | integer | Unix timestamp when this listing was generated |

The file listing is cached for 30 seconds. Subsequent requests within that window return the cached result.

### File Streaming

```
GET /api/v1/files/{shared_entry_hash}/{relative_path}[?download=1]
```

Streams a file over HTTP with full Range support.

| Parameter | Location | Description |
|-----------|----------|-------------|
| `shared_entry_hash` | URL path | 56-char hex hash from the file listing |
| `relative_path` | URL path | Path to the file within the shared directory (URL-encoded) |
| `download` | Query string | Set to `1` to force a download dialog instead of inline display |

**Example — inline streaming:**
```http
GET /api/v1/files/aabb1122.../Videos/movie.mp4 HTTP/1.1
Host: 192.168.1.100:8080
```
```http
HTTP/1.1 200 OK
Content-Type: video/mp4
Content-Length: 15728640
Content-Disposition: inline; filename="movie.mp4"
Accept-Ranges: bytes
ETag: "aabb112233445566:1712345678:15728640"
Cache-Control: private, max-age=60
Connection: close

<file bytes>
```

**Example — forced download:**
```http
GET /api/v1/files/aabb1122.../game.zip?download=1 HTTP/1.1
```
```http
HTTP/1.1 200 OK
Content-Disposition: attachment; filename="game.zip"
...
```

| `?download=` | `Content-Disposition` | Browser Behavior |
|-------------|----------------------|-----------------|
| *(omitted)* | `inline; filename="..."` | Browser displays/plays if it can, or downloads |
| `1` | `attachment; filename="..."` | Browser always opens a save dialog |

### Status

```
GET /api/v1/status
GET /api/status       (backward-compatible alias)
```

Core connection status, indexing progress, and peer count.

**Response:**
```json
{
  "connected": true,
  "peer": "0011aabbccdd...56_hex_chars",
  "cache_status": "UP_TO_DATE",
  "cache_progress": 100.0,
  "shared_entries": 3,
  "peers_online": 5
}
```

| Field | Type | Description |
|-------|------|-------------|
| `connected` | boolean | Whether the server is connected to Core |
| `peer` | string | This Core's peer ID (empty if disconnected) |
| `cache_status` | string | File index cache state |
| `cache_progress` | number | Indexing progress (0.0 – 100.0) |
| `shared_entries` | integer | Number of shared directories |
| `peers_online` | integer | Number of peers currently connected |

### Health Check

```
GET /api/v1/health
GET /api/health       (backward-compatible alias)
```

Simple liveness probe.

**Response:**
```json
{"status": "ok"}
```

---

## HTTP Features

### Range Requests

The streaming endpoint fully supports [RFC 7233](https://datatracker.ietf.org/doc/html/rfc7233) Range requests:

- **Resume interrupted downloads** — `Range: bytes=1048576-`
- **Seek within video/audio** — media players use Range to jump to a timestamp.
- **Fetch a slice** — `Range: bytes=0-1023` for just the first 1 KiB.

Three range formats are supported:

| Format | Example | Meaning |
|--------|---------|---------|
| Start–End | `bytes=0-1023` | Bytes 0 through 1023 (1 KiB) |
| Start–EOF | `bytes=1048576-` | From byte 1 MiB to end of file |
| Suffix | `bytes=-1024` | Last 1024 bytes |

**Example:**
```http
GET /api/v1/files/aabb.../movie.mkv HTTP/1.1
Range: bytes=0-1023
```
```http
HTTP/1.1 206 Partial Content
Content-Range: bytes 0-1023/1073741824
Content-Length: 1024
Accept-Ranges: bytes
```

Invalid ranges return `416 Range Not Satisfiable`. Multi-part ranges are not supported.

### ETag Caching

Every file response includes an `ETag` header for conditional caching.

**ETag format:** `"{hash_prefix}:{mtime}:{size}"`
- `hash_prefix` — first 16 characters of the shared entry hash
- `mtime` — file modification time (Unix seconds)
- `size` — file size in bytes

**Example:** `"aabb112233445566:1712345678:1073741824"`

**Conditional request (cache hit):**
```http
GET /api/v1/files/aabb.../movie.mkv HTTP/1.1
If-None-Match: "aabb112233445566:1712345678:1073741824"
```
```http
HTTP/1.1 304 Not Modified
ETag: "aabb112233445566:1712345678:1073741824"
```

Browsers handle `If-None-Match` automatically — no JavaScript needed. Additionally,
responses include `Cache-Control: private, max-age=60` for short-term browser caching.

### Content-Type Detection

MIME types are detected from file extensions using Python's `mimetypes` module.
Common mappings:

| Extension | Content-Type | Browser Behavior |
|-----------|-------------|-----------------|
| `.mp4` | `video/mp4` | Plays inline |
| `.mkv` | `video/x-matroska` | Plays inline (Chromium) |
| `.mp3` | `audio/mpeg` | Plays inline |
| `.flac` | `audio/flac` | Plays inline |
| `.jpg`, `.jpeg` | `image/jpeg` | Displays inline |
| `.png` | `image/png` | Displays inline |
| `.pdf` | `application/pdf` | Displays inline |
| `.zip` | `application/zip` | Download prompt |
| `.exe` | `application/x-msdownload` | Download prompt |
| `.txt` | `text/plain` | Displays inline |
| *(unknown)* | `application/octet-stream` | Download prompt |

### Content-Disposition

By default, files are served with `Content-Disposition: inline`, letting the browser
decide how to handle them. Use `?download=1` to force a save dialog:

| URL | Header | Result |
|-----|--------|--------|
| `/api/v1/files/aabb.../photo.jpg` | `inline; filename="photo.jpg"` | Displays in browser |
| `/api/v1/files/aabb.../photo.jpg?download=1` | `attachment; filename="photo.jpg"` | Opens save dialog |

Filenames with special characters are sanitized: double quotes, carriage returns,
and newlines are stripped to prevent HTTP response splitting.

### CORS

Cross-Origin Resource Sharing is **not enabled by default**. Configure it explicitly:

```bash
python server.py --cors-origins https://mysite.com
```

When configured, these headers are included in responses:
```http
Access-Control-Allow-Origin: https://mysite.com
Access-Control-Allow-Methods: GET, OPTIONS
Access-Control-Allow-Headers: Content-Type
Access-Control-Max-Age: 86400
```

For development, allow all origins:
```bash
python server.py --cors-origins "*"
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

`buildStreamUrl()` constructs HTTP streaming URLs from file listing data:

```javascript
const data = await api.getFiles();

for (const file of data.files) {
  // Inline/playback URL
  const url = api.buildStreamUrl(file);
  // → "http://192.168.1.100:8080/api/v1/files/aabb.../movie.mkv"

  // Forced download URL
  const downloadUrl = api.buildStreamUrl({ ...file, download: true });
  // → "http://192.168.1.100:8080/api/v1/files/aabb.../movie.mkv?download=1"
}
```

| Property | Required | Description |
|----------|----------|-------------|
| `shared_entry_hash` | Yes | 56-char hex shared entry identifier |
| `name` | Yes | Filename |
| `path` | No | Directory path within the shared entry (`/` for root) |
| `download` | No | `true` to append `?download=1` |

Path components are URL-encoded automatically (spaces, special characters).

### Building dglan:// Links

For native DG-LAN downloads (faster, multi-source):

```javascript
// File listing already includes pre-built links
const link = data.files[0].dglan_url;
// → "dglan://download?peer=0011aa...&hash=aabb11...&size=1073741824&name=movie.mkv&path=%2F"

// Or build manually
const link = DglanApi.buildLink({
  peer: file.peer,
  hash: file.shared_entry_hash,
  size: file.size,
  name: file.name,
  path: file.path
});
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
        dl.textContent = "\u2B07";
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

### Video and Audio Embedding

Range requests enable native `<video>` and `<audio>` elements to seek freely:

```html
<!-- Stream video — browser handles Range for seeking -->
<video controls width="720">
  <source src="http://192.168.1.100:8080/api/v1/files/aabb.../movie.mp4" type="video/mp4">
</video>

<!-- Audio player -->
<audio controls>
  <source src="http://192.168.1.100:8080/api/v1/files/aabb.../song.mp3" type="audio/mpeg">
</audio>
```

For programmatic Range usage:
```javascript
const url = api.buildStreamUrl(file);
const resp = await fetch(url, { headers: { Range: "bytes=0-999" } });
console.log(resp.status);                          // 206
console.log(resp.headers.get("Content-Range"));    // "bytes 0-999/15728640"
```

---

## URL Schemes

### HTTP Streaming URLs

```
/api/v1/files/{shared_entry_hash}/{relative_path}[?download=1]
 ─────┬──────  ────────┬────────  ──────┬───────   ─────┬─────
       │               │                │               │
  API version    56-char hex       Path inside       Optional
  prefix         SHA3-224 hash     shared dir        query param
                 of the shared
                 directory
```

| URL | Description |
|-----|-------------|
| `/api/v1/files/aabb.../readme.txt` | File at root of shared directory |
| `/api/v1/files/aabb.../Games/doom.zip` | File in a subdirectory |
| `/api/v1/files/aabb.../My%20Files/doc.pdf` | Space in path (URL-encoded) |
| `/api/v1/files/aabb.../game.iso?download=1` | Force download dialog |

### dglan:// Download URLs

For native DG-LAN downloads (requires DG-LAN client installed):

```
dglan://download?peer=PEER_HEX&hash=ENTRY_HEX&size=BYTES&name=FILENAME&path=/
```

| Parameter | Description |
|-----------|-------------|
| `peer` | 56-char hex peer ID |
| `hash` | 56-char hex shared entry ID |
| `size` | File size in bytes |
| `name` | Percent-encoded filename |
| `path` | Percent-encoded directory path (`/` for root) |

The file listing endpoint returns pre-built `dglan_url` values — use those directly.

---

## Security

### Path Traversal Protection

The server uses a multi-step defense:

1. **Hash validation** — shared entry hash must match `^[0-9a-f]{56}$` (exactly 56 lowercase hex characters).
2. **Null byte rejection** — paths containing `\x00` are rejected immediately.
3. **Shared entry lookup** — hash is checked against known shared directories. Unknown hashes are rejected.
4. **Canonicalization** — both the shared directory root and the requested path are resolved via `os.path.realpath()` (resolves symlinks, `..`, etc.).
5. **Common path check** — `os.path.commonpath()` verifies the resolved file path shares the same root as the shared directory.
6. **File type check** — only regular files are served (not directories, symlinks to directories, or special files).

These attacks are all blocked:
```
/api/v1/files/aabb.../../../etc/passwd
/api/v1/files/aabb.../..%2F..%2Fetc%2Fpasswd
/api/v1/files/aabb.../valid/../../../secret
/api/v1/files/aabb.../C:%5CWindows%5Csystem32
```

### Hash Validation

The shared entry hash must be exactly 56 lowercase hexadecimal characters
(SHA3-224). Any other format is rejected with `400 INVALID_HASH`.

### Header Injection Prevention

Filenames in `Content-Disposition` headers are sanitized: double quotes,
carriage returns, and newlines are stripped to prevent HTTP response splitting.

### Deployment Recommendations

The Python bridge does **not** include TLS. For production:

```
Internet → nginx (HTTPS, :443) → dglan-api (HTTP, 127.0.0.1:8080)
```

**nginx:**
```nginx
server {
    listen 443 ssl;
    server_name files.example.com;

    ssl_certificate     /etc/ssl/cert.pem;
    ssl_certificate_key /etc/ssl/key.pem;

    location /api/v1/files/ {
        proxy_pass http://127.0.0.1:8080;
        proxy_buffering off;
        proxy_request_buffering off;
        proxy_http_version 1.1;
        client_max_body_size 0;
    }

    location /api/ {
        proxy_pass http://127.0.0.1:8080;
    }
}
```

Key nginx settings for file streaming:
- `proxy_buffering off` — don't buffer the entire file in nginx memory.
- `client_max_body_size 0` — no request size limit.
- `proxy_http_version 1.1` — enables keep-alive to the backend.

**Caddy:**
```
files.example.com {
    reverse_proxy /api/* 127.0.0.1:8080 {
        flush_interval -1
    }
}
```

---

## Configuration

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
| `--verbose` / `-v` | off | Enable debug logging |

### Environment Variables

| Variable | Description |
|----------|-------------|
| `DGLAN_PASSWORD` | Core remote password (more secure than CLI flag — avoids process list exposure) |

### Binding to All Interfaces

By default the server only listens on localhost. To allow network access:

```bash
python server.py --http-host 0.0.0.0
```

> **Warning:** This exposes all shared files to anyone on your network.
> Combine with `--cors-origins` and a reverse proxy for controlled access.

### Reverse Proxy (Production)

See [Deployment Recommendations](#deployment-recommendations) above for nginx and Caddy examples.

### Running as a Service

**Windows (Task Scheduler):**
```powershell
$Action = New-ScheduledTaskAction -Execute "python" `
  -Argument "server.py --http-host 0.0.0.0" `
  -WorkingDirectory "C:\DG-LAN\dglan-api"
$Trigger = New-ScheduledTaskTrigger -AtStartup
Register-ScheduledTask -TaskName "DG-LAN API" -Action $Action -Trigger $Trigger -RunLevel Highest
```

**Linux (systemd):**
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

---

## Error Reference

All errors return a consistent JSON format:

```json
{
  "error": {
    "code": "ERROR_CODE",
    "message": "Human-readable description"
  }
}
```

| HTTP Status | Error Code | Cause | Resolution |
|-------------|-----------|-------|------------|
| 400 | `INVALID_PATH` | Missing or empty file path after the hash | Include a filename: `/api/v1/files/{hash}/filename.ext` |
| 400 | `INVALID_HASH` | shared_entry_hash is not 56 lowercase hex characters | Use the exact hash from the file listing |
| 404 | `NOT_FOUND` | Unknown route | Check URL path spelling |
| 404 | `ENTRY_NOT_FOUND` | No shared directory matches that hash | Refresh the file listing (`GET /api/v1/files`) |
| 404 | `FILE_NOT_FOUND` | File doesn't exist at that path within the shared directory | Check spelling, path separators, and URL encoding |
| 405 | `METHOD_NOT_ALLOWED` | Used POST, PUT, DELETE, etc. | Only GET and OPTIONS are supported |
| 416 | *(standard)* | Range header requests bytes beyond file bounds | Use `HEAD` first to get `Content-Length` |
| 500 | `IO_ERROR` | Disk read failure while streaming | Check file permissions and disk health |
| 500 | `INTERNAL_ERROR` | Unexpected server error | Check server output (`--verbose`) |
| 503 | `CORE_DISCONNECTED` | Server lost connection to DG-LAN Core | Ensure Core is running; server reconnects automatically |

---

## Troubleshooting

### "CORE_DISCONNECTED" error

The server lost its TCP connection to Core.

1. Is Core running? Check for `DG-LAN.Core.exe` in Task Manager or the DG-LAN GUI.
2. The server reconnects automatically — wait 5 seconds and retry.
3. If using `--core-host` for a remote Core, verify network connectivity and password.

### File returns 404 but it shows in the file listing

1. **URL encoding** — spaces must be `%20`: `My File.txt` → `My%20File.txt`.
2. **Case sensitivity** — Windows paths are case-insensitive but the URL must match the listing.
3. **Stale cache** — the file listing is cached for 30 seconds. If you just added a shared directory, wait and re-fetch the listing.

### Video won't seek / shows loading spinner

The reverse proxy may be stripping the `Range` header or buffering the response.

In nginx, add `proxy_buffering off;` to the streaming location block.

### CORS errors in browser console

```
Access to fetch at 'http://...' has been blocked by CORS policy
```

Start the server with the correct origin:
```bash
python server.py --cors-origins https://your-website.com
```

For development only:
```bash
python server.py --cors-origins "*"
```

### Large files are slow

The server streams files in 64 KiB chunks (same as the built-in server).
For very large files served to many concurrent users:

1. Put nginx in front with `sendfile on;` and `tcp_nopush on;`.
2. Ensure the disk can sustain the required I/O throughput.
3. Consider using the [built-in HTTP server](../HTTP-SERVER.md) (port 59480) which avoids the Python-to-Core TCP overhead.

---

## Testing

### Manual Testing

**1. Start the server:**
```bash
cd dglan-api
python server.py --verbose
```

**2. Health check:**
```bash
curl http://127.0.0.1:8080/api/v1/health
# {"status": "ok"}
```

**3. Get the file listing:**
```bash
curl http://127.0.0.1:8080/api/v1/files | python -m json.tool
```

**4. Stream a file** (use an `http_url` from the listing):
```bash
curl -o output.mkv http://127.0.0.1:8080/api/v1/files/aabb.../movie.mkv
```

**5. Test Range request:**
```bash
curl -v -H "Range: bytes=0-1023" http://127.0.0.1:8080/api/v1/files/aabb.../movie.mkv
# Should return: HTTP/1.1 206 Partial Content
```

**6. Test ETag conditional request:**
```bash
ETAG=$(curl -sI http://127.0.0.1:8080/api/v1/files/aabb.../movie.mkv | grep -i etag | tr -d '\r' | cut -d' ' -f2)
curl -v -H "If-None-Match: $ETAG" http://127.0.0.1:8080/api/v1/files/aabb.../movie.mkv
# Should return: HTTP/1.1 304 Not Modified
```

**7. Test forced download:**
```bash
curl -v "http://127.0.0.1:8080/api/v1/files/aabb.../movie.mkv?download=1"
# Content-Disposition should say "attachment"
```

**8. Open in browser:**
Paste any `http_url` (with server base URL) into your browser. Video/audio plays inline, images display, other files download.

### Automated Tests

The file streamer has 59 unit tests covering all code paths. No running Core required.

```bash
cd dglan-api
pip install pytest
pytest test_streamer.py -v
```

| Test Class | Count | Coverage |
|------------|-------|----------|
| `TestHashPattern` | 7 | SHA3-224 hash format validation |
| `TestResolve` | 12 | Path resolution, traversal prevention, edge cases |
| `TestParseRange` | 14 | RFC 7233 Range header parsing, suffix ranges, clamping |
| `TestComputeEtag` | 3 | ETag generation from file metadata |
| `TestGuessContentType` | 6 | MIME type detection for common file types |
| `TestStream` | 13 | Full streaming: 200, 206, 304, 416, Content-Disposition |
| `TestHTTPRouting` | 4 | URL routing: valid paths, missing paths, bad hashes |

All tests use temporary files and mock objects — they run in under a second and
require no network access or DG-LAN Core instance.

---

*See also: [Built-in HTTP Server](../HTTP-SERVER.md) for the C++ server embedded in Core, [README.md](README.md) for Python bridge setup, [TESTING.md](TESTING.md) for the testing guide.*
