# Built-in HTTP File Server

DG-LAN Core includes a built-in HTTP file server that lets any device on your network
access shared files through a standard web browser — no DG-LAN client installation required.

---

## Table of Contents

- [Why a Built-in HTTP Server?](#why-a-built-in-http-server)
- [How It Works](#how-it-works)
  - [Architecture](#architecture)
  - [Load Balancing (Already Done)](#load-balancing-already-done)
  - [Peer Discovery and Redirect](#peer-discovery-and-redirect)
- [Getting Started](#getting-started)
- [Configuration](#configuration)
  - [Settings](#settings)
  - [Ports Reference](#ports-reference)
- [API Reference](#api-reference)
  - [File Streaming — GET /files/{hash}/{path}](#file-streaming)
  - [File Listing — GET /api/v1/files](#file-listing)
  - [Network Status — GET /api/v1/status](#network-status)
  - [Health Check — GET /api/v1/health](#health-check)
- [HTTP Features](#http-features)
  - [Range Requests (Resume & Seek)](#range-requests)
  - [ETag Caching](#etag-caching)
  - [Content-Type Detection](#content-type-detection)
  - [CORS](#cors)
  - [HEAD Requests](#head-requests)
- [Use Cases & Best Practices](#use-cases--best-practices)
  - [Stream Video in a Browser](#stream-video-in-a-browser)
  - [Embed Audio on a Web Page](#embed-audio-on-a-web-page)
  - [Build a File Download Page](#build-a-file-download-page)
  - [Integrate with JavaScript](#integrate-with-javascript)
- [Security](#security)
  - [Path Traversal Protection](#path-traversal-protection)
  - [Input Validation](#input-validation)
  - [Connection Limits](#connection-limits)
  - [Deployment Recommendations](#deployment-recommendations)
- [Error Reference](#error-reference)
- [Troubleshooting](#troubleshooting)
- [Comparison: Built-in Server vs Python Bridge](#comparison-built-in-server-vs-python-bridge)

---

## Why a Built-in HTTP Server?

DG-LAN's native protocol (`dglan://`) requires the DG-LAN client to be installed.
That's fine for LAN party attendees who want maximum download speed — but sometimes
you just want to share a file with someone who has nothing installed.

The built-in HTTP server solves this:

- **Zero installation** — any device with a web browser can access files.
- **Zero configuration** — enabled by default, runs on port 59480.
- **No external dependencies** — it's compiled into `DG-LAN.Core.exe`, not a separate process.
- **Full HTTP semantics** — Range requests for video seeking, ETag caching, proper MIME types.
- **Peer redirect** — if a file isn't on this machine, the server redirects the browser to a peer that has it.

The native DG-LAN protocol remains the fastest way to download (multi-source chunked transfer),
but HTTP gives universal access to anyone on the network.

---

## How It Works

### Architecture

```
┌──────────────────────────────────────────────────────────────┐
│  DG-LAN Core  (DG-LAN.Core.exe / Windows service)           │
│                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────┐ │
│  │ FileManager  │  │ PeerManager  │  │ HttpServer         │ │
│  │  File index  │  │  Peer list   │  │  QTcpServer        │ │
│  │  Disk I/O    │  │  IP + ports  │  │  :59480            │ │
│  └──────┬───────┘  └──────┬───────┘  └────────┬───────────┘ │
│         │                 │                    │             │
│         └─────────────────┴────────────────────┘             │
│                           │                                  │
└───────────────────────────┼──────────────────────────────────┘
                            │
                       HTTP requests
                            │
                    ┌───────▼───────┐
                    │   Browser /   │
                    │   curl / app  │
                    └───────────────┘
```

The HttpServer module is a static library linked directly into Core. It starts
automatically when Core starts (if enabled in settings). It has direct in-process
access to the FileManager (file index and disk I/O) and PeerManager (peer list
with IP addresses and HTTP ports).

### Load Balancing (Already Done)

DG-LAN's load balancing happens at the **file download layer**, not the HTTP layer.
By the time a file is available through the HTTP server, it's already been assembled
from chunks pulled in parallel from multiple peers:

```
File: game-installer.exe (256 MiB)

Peer A ──► Chunk 1 (64 MiB) ──┐
Peer B ──► Chunk 2 (64 MiB) ──┤
Peer C ──► Chunk 3 (64 MiB) ──├──► Complete file on disk
Peer A ──► Chunk 4 (64 MiB) ──┘     ↓
                                  HTTP server streams it
```

This means:
- The HTTP server serves files at **full local disk speed** — there's no network bottleneck.
- Multiple HTTP clients can stream the same file simultaneously (standard OS file I/O).
- The load balancing already happened — no peer coordination during HTTP serving.

### Peer Discovery and Redirect

Each DG-LAN Core broadcasts its HTTP port in the periodic IMAlive heartbeat messages
(UDP multicast/broadcast/unicast). All peers on the network know each other's HTTP ports.

When a browser requests a file that this machine doesn't have, the server picks a
master peer that has HTTP enabled and sends a **302 redirect**:

```
Browser → Machine A:59480    "GET /files/abc.../movie.mkv"
                              File not found locally
Machine A → Browser           "302 Found → http://192.168.1.50:59480/files/abc.../movie.mkv"
Browser → Machine B:59480    "GET /files/abc.../movie.mkv"  (automatic redirect)
Machine B → Browser           "200 OK" + file stream
```

This is transparent to the user. Peers with HTTP disabled (port = 0) are skipped.

---

## Getting Started

The HTTP server is **enabled by default**. Once DG-LAN Core is running with shared
folders configured, files are immediately accessible:

```bash
# List all shared files
curl http://localhost:59480/api/v1/files

# Stream a file (use a shared_entry_hash from the listing)
curl http://localhost:59480/files/aabb1122.../movie.mkv -o movie.mkv

# Open in a browser
# → http://192.168.1.100:59480/files/aabb1122.../photo.jpg
```

No setup steps, no additional processes, no Python — it just works.

---

## Configuration

### Settings

The HTTP server is configured through Core's protobuf settings (stored in the
Core's persistent settings file). These are the defaults:

| Setting | Default | Description |
|---------|---------|-------------|
| `http_server_enabled` | `true` | Enable or disable the HTTP server |
| `http_server_port` | `59480` | TCP port to listen on |
| `http_max_connections` | `50` | Maximum simultaneous connections |

Port 59480 was chosen to sit within the DG-LAN port family (59485 = remote control,
59486 = multicast, 59487 = unicast) while staying out of the way of common services.

The HTTP port is also broadcast in the IMAlive peer discovery protocol (field `http_port`
in the `IMAlive` protobuf message), so every peer on the network knows how to reach
every other peer's HTTP server.

### Ports Reference

| Port | Protocol | Purpose |
|------|----------|---------|
| **59480** | **TCP** | **Built-in HTTP file server** |
| 59485 | TCP | GUI ↔ Core remote control |
| 59486 | UDP | Multicast / broadcast peer discovery |
| 59487 | UDP + TCP | Unicast peer communication |

If you need to allow HTTP access through a firewall:

```powershell
# Windows Firewall example
netsh advfirewall firewall add rule name="DG-LAN HTTP" dir=in action=allow protocol=TCP localport=59480
```

---

## API Reference

### File Streaming

```
GET /files/{shared_entry_hash}/{relative_path}
```

Streams a file directly over HTTP with full Range support. This is the primary
endpoint — use it for browser playback, downloads, and embedding.

| Parameter | Location | Description |
|-----------|----------|-------------|
| `shared_entry_hash` | URL path | Hex-encoded hash identifying the shared directory (see [File Listing](#file-listing)) |
| `relative_path` | URL path | Path to the file within the shared directory (URL-encoded) |

**Example request:**
```http
GET /files/a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4/Videos/movie.mp4 HTTP/1.1
Host: 192.168.1.100:59480
```

**Example response (200 OK):**
```http
HTTP/1.1 200 OK
Content-Type: video/mp4
Content-Length: 15728640
Accept-Ranges: bytes
ETag: "a1b2c3d4e5f6a1b2"
Access-Control-Allow-Origin: *
Access-Control-Expose-Headers: Content-Length, Content-Range, ETag, Accept-Ranges
Content-Disposition: inline; filename="movie.mp4"
Connection: close

<file bytes>
```

**Behavior:**
- Files are always served with `Content-Disposition: inline` (browser decides whether to display or download based on content type).
- If the file is not found locally, the server attempts a [peer redirect](#peer-discovery-and-redirect) (302).
- If no peer has the file either, returns 404.

### File Listing

```
GET /api/v1/files
```

Returns a JSON listing of all files in the shared index, with pre-built HTTP URLs
and peer URLs for every master peer on the network.

**Response:**
```json
{
  "files": [
    {
      "name": "Movies",
      "size": 53687091200,
      "is_directory": true,
      "shared_entry_hash": "a1b2c3d4e5f6...28_hex_bytes",
      "http_url": "/files/a1b2c3d4e5f6...28_hex_bytes/",
      "peer_urls": [
        "http://192.168.1.50:59480/files/a1b2c3d4e5f6...28_hex_bytes/",
        "http://192.168.1.51:59480/files/a1b2c3d4e5f6...28_hex_bytes/"
      ]
    }
  ],
  "peer_id": "0011aabbccdd...28_hex_bytes"
}
```

| Field | Type | Description |
|-------|------|-------------|
| `files` | array | All entries in the shared file index |
| `files[].name` | string | Entry name |
| `files[].size` | integer | Size in bytes |
| `files[].is_directory` | boolean | `true` for directories, `false` for files |
| `files[].shared_entry_hash` | string | Hex-encoded hash — use this in `/files/{hash}/{path}` URLs |
| `files[].http_url` | string | Pre-built relative URL for this entry |
| `files[].peer_urls` | array | Full HTTP URLs for every master peer with HTTP enabled |
| `peer_id` | string | This peer's identifier (hex-encoded) |

**`peer_urls`** contains one URL per master peer on the network that has HTTP enabled
(i.e., peers whose `http_port` > 0 in their IMAlive broadcast). Use these to access
the file directly from a specific peer.

### Network Status

```
GET /api/v1/status
```

Returns the current network state — peer list with connection details.

**Response:**
```json
{
  "version": "1.2.97 Alpha",
  "peer_count": 3,
  "peers": [
    {
      "nick": "GameServer",
      "ip": "192.168.1.50",
      "sharing_amount": 107374182400,
      "download_rate": 0,
      "upload_rate": 52428800,
      "is_master": true
    }
  ]
}
```

| Field | Type | Description |
|-------|------|-------------|
| `version` | string | Core version string |
| `peer_count` | integer | Number of known peers |
| `peers[].nick` | string | Peer nickname |
| `peers[].ip` | string | Peer IP address |
| `peers[].sharing_amount` | integer | Total bytes shared by this peer |
| `peers[].download_rate` | integer | Current download rate (bytes/sec) |
| `peers[].upload_rate` | integer | Current upload rate (bytes/sec) |
| `peers[].is_master` | boolean | Whether this peer is a network master |

### Health Check

```
GET /api/v1/health
```

Simple liveness probe for monitoring and load balancer integration.

**Response:**
```json
{
  "status": "ok",
  "version": "1.2.97 Alpha"
}
```

---

## HTTP Features

### Range Requests

The file streaming endpoint fully supports [RFC 7233](https://datatracker.ietf.org/doc/html/rfc7233)
Range requests. This enables:

- **Video/audio seeking** — media players use Range to jump to a specific timestamp without downloading the whole file.
- **Download resume** — interrupted downloads can continue from where they stopped.
- **Partial fetches** — retrieve only a slice of a large file.

Three range formats are supported:

**Byte range (start to end, inclusive):**
```http
GET /files/aabb.../movie.mkv HTTP/1.1
Range: bytes=0-1023
```
```http
HTTP/1.1 206 Partial Content
Content-Range: bytes 0-1023/15728640
Content-Length: 1024
```

**Open-ended range (start to end of file):**
```http
Range: bytes=1048576-
```
```http
HTTP/1.1 206 Partial Content
Content-Range: bytes 1048576-15728639/15728640
Content-Length: 14680064
```

**Suffix range (last N bytes):**
```http
Range: bytes=-1024
```
```http
HTTP/1.1 206 Partial Content
Content-Range: bytes 15727616-15728639/15728640
Content-Length: 1024
```

**Invalid ranges** return `416 Range Not Satisfiable`. Range validation:
- Start position must be within file bounds.
- End position must not precede start.
- End position must be within file bounds (not clamped — strict validation).
- Suffix length must not exceed file size.
- Multi-part ranges (e.g., `bytes=0-100,200-300`) are not supported.

### ETag Caching

Every file response includes an `ETag` header. Clients can use `If-None-Match`
to avoid re-downloading unchanged files.

**How the ETag is computed:**

The server hashes three values using MD5 and takes the first 16 hex characters:

```
ETag = MD5(file_path + file_size + last_modified_unix_seconds)[0:16]
```

This means the ETag changes if the file is renamed, resized, or modified.

**First request:**
```http
GET /files/aabb.../movie.mkv HTTP/1.1
```
```http
HTTP/1.1 200 OK
ETag: "a3f2b8c91e4d7a06"
```

**Subsequent request (cache hit):**
```http
GET /files/aabb.../movie.mkv HTTP/1.1
If-None-Match: "a3f2b8c91e4d7a06"
```
```http
HTTP/1.1 304 Not Modified
ETag: "a3f2b8c91e4d7a06"
```

Browsers handle this automatically — no JavaScript needed.

### Content-Type Detection

The server detects MIME types from file extensions. This determines how browsers
handle the file (play inline, display, or prompt for download).

| Extension | Content-Type | Browser Behavior |
|-----------|-------------|------------------|
| `.mp4`, `.m4v` | `video/mp4` | Plays inline |
| `.mkv` | `video/x-matroska` | Plays inline (Chrome/Edge) |
| `.webm` | `video/webm` | Plays inline |
| `.avi` | `video/x-msvideo` | Download prompt |
| `.mov` | `video/quicktime` | Plays inline (Safari) |
| `.mp3` | `audio/mpeg` | Plays inline |
| `.flac` | `audio/flac` | Plays inline (most browsers) |
| `.ogg` | `audio/ogg` | Plays inline |
| `.wav` | `audio/wav` | Plays inline |
| `.m4a` | `audio/mp4` | Plays inline |
| `.png` | `image/png` | Displays inline |
| `.jpg`, `.jpeg` | `image/jpeg` | Displays inline |
| `.gif` | `image/gif` | Displays inline |
| `.webp` | `image/webp` | Displays inline |
| `.svg` | `image/svg+xml` | Displays inline |
| `.ico` | `image/x-icon` | Displays inline |
| `.zip` | `application/zip` | Download prompt |
| `.7z` | `application/x-7z-compressed` | Download prompt |
| `.rar` | `application/x-rar-compressed` | Download prompt |
| `.tar` | `application/x-tar` | Download prompt |
| `.gz` | `application/gzip` | Download prompt |
| `.iso` | `application/x-iso9660-image` | Download prompt |
| `.pdf` | `application/pdf` | Displays inline |
| `.txt`, `.log`, `.md` | `text/plain; charset=utf-8` | Displays inline |
| `.html`, `.htm` | `text/html; charset=utf-8` | Renders as page |
| `.css` | `text/css; charset=utf-8` | Displays as text |
| `.js` | `application/javascript` | Download prompt |
| `.json` | `application/json` | Displays inline |
| `.xml` | `application/xml` | Displays inline |
| `.exe`, `.msi` | `application/octet-stream` | Download prompt |
| *(unknown)* | `application/octet-stream` | Download prompt |

### CORS

All responses include permissive CORS headers, allowing any website to fetch files:

```http
Access-Control-Allow-Origin: *
Access-Control-Expose-Headers: Content-Length, Content-Range, ETag, Accept-Ranges
```

**OPTIONS preflight** responses (for cross-origin requests from JavaScript):
```http
HTTP/1.1 204 No Content
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: GET, HEAD, OPTIONS
Access-Control-Allow-Headers: Range, If-None-Match
Access-Control-Max-Age: 86400
```

The `Max-Age: 86400` (24 hours) means browsers cache the preflight result,
so only the first cross-origin request from a given page triggers a preflight.

### HEAD Requests

All endpoints support `HEAD` requests — returns headers only, no body.
Useful for checking file size, content type, or ETag without downloading.

```bash
curl -I http://192.168.1.100:59480/files/aabb.../movie.mkv
```
```http
HTTP/1.1 200 OK
Content-Type: video/mp4
Content-Length: 15728640
Accept-Ranges: bytes
ETag: "a3f2b8c91e4d7a06"
```

---

## Use Cases & Best Practices

### Stream Video in a Browser

Paste this URL into any browser on the LAN:
```
http://192.168.1.100:59480/files/aabb.../Videos/movie.mp4
```

The video plays inline with full seeking support (thanks to Range requests).
Works with `.mp4`, `.webm`, and `.mkv` (in Chromium-based browsers).

### Embed Audio on a Web Page

```html
<audio controls>
  <source src="http://192.168.1.100:59480/files/aabb.../Music/song.mp3" type="audio/mpeg">
</audio>
```

### Build a File Download Page

Fetch the file listing from the API, then generate HTML links:

```html
<script>
  fetch('http://192.168.1.100:59480/api/v1/files')
    .then(r => r.json())
    .then(data => {
      for (const file of data.files) {
        const a = document.createElement('a');
        a.href = `http://192.168.1.100:59480${file.http_url}`;
        a.textContent = `${file.name} (${file.size} bytes)`;
        document.body.appendChild(a);
        document.body.appendChild(document.createElement('br'));
      }
    });
</script>
```

### Integrate with JavaScript

The `dglan-api.js` client library works with both the built-in server and the
Python bridge. The `buildStreamUrl()` method generates correct URLs:

```javascript
const api = new DglanApi("http://192.168.1.100:59480");
const data = await api.getFiles();

for (const file of data.files) {
  const url = api.buildStreamUrl(file);
  // → "http://192.168.1.100:59480/api/v1/files/aabb.../path/to/file.ext"
  console.log(url);
}
```

> **Note:** `buildStreamUrl()` generates paths under `/api/v1/files/{hash}/{path}`.
> The built-in server routes file streaming under `/files/{hash}/{path}` (no `/api/v1` prefix).
> Use the `http_url` field from the file listing response directly when working with
> the built-in server, as it already contains the correct `/files/` prefix.

---

## Security

### Path Traversal Protection

The server uses a multi-layered defense against directory traversal attacks:

1. **Hash format validation** — the shared entry hash must match `^[0-9a-fA-F]{1,64}$`.
2. **Null byte rejection** — paths containing `\0` are rejected immediately.
3. **Shared entry lookup** — the hash is decoded and looked up in the FileManager. Only registered shared directories are accessible.
4. **Path cleaning** — `QDir::cleanPath()` removes `.` and `..` components.
5. **Canonicalization** — `QFileInfo::canonicalFilePath()` resolves symlinks and any remaining `..` traversals to the real absolute path.
6. **Prefix check** — the canonical file path must start with the canonical shared directory path.
7. **File type check** — only regular files are served (not directories, devices, or pipes).

All of these attacks are blocked and return 404:
```
/files/aabb.../../../etc/passwd
/files/aabb.../..%2F..%2Fetc%2Fpasswd
/files/aabb.../valid/../../../secret
/files/aabb.../C:%5CWindows%5Csystem32
```

### Input Validation

- **Request header size** is capped at **8 KiB**. Requests exceeding this return `413 Payload Too Large`.
- **HTTP methods** are restricted to `GET`, `HEAD`, and `OPTIONS`. All others return `405`.
- Hash parameters must be hexadecimal (1–64 characters).

### Connection Limits

The server enforces a maximum of **50 simultaneous connections** (configurable via
`http_max_connections`). When the limit is reached, new connections receive:

```http
HTTP/1.1 503 Service Unavailable
Connection: close
```

This prevents resource exhaustion from connection floods.

### Deployment Recommendations

The built-in server is designed for **LAN use**. It does not include TLS,
authentication, or rate limiting. For internet-facing deployments:

1. **Put it behind a reverse proxy** (nginx, Caddy) with HTTPS.
2. **Restrict access** via firewall rules to trusted networks.
3. **Add authentication** at the proxy layer if needed.

```nginx
# nginx reverse proxy example
server {
    listen 443 ssl;
    server_name files.example.com;

    location /files/ {
        proxy_pass http://127.0.0.1:59480;
        proxy_buffering off;           # Don't buffer large files
        proxy_http_version 1.1;
    }

    location /api/ {
        proxy_pass http://127.0.0.1:59480;
    }
}
```

---

## Error Reference

All errors return JSON:

```json
{
  "error": {
    "code": 404,
    "message": "File not found"
  }
}
```

| HTTP Status | Message | Cause | Resolution |
|-------------|---------|-------|------------|
| 400 | `Invalid path: expected /files/{hash}/{path}` | Missing hash or path in URL | Include both: `/files/{hash}/{filename}` |
| 400 | `Missing file path` | Hash present but no filename after it | Add a filename: `/files/{hash}/file.ext` |
| 404 | `Not found` | Unknown route | Check the URL path |
| 404 | `File not found` | File doesn't exist at the given path and no peer could serve it | Verify: refresh the file listing, check spelling and URL encoding |
| 405 | `Method not allowed` | Used POST, PUT, DELETE, etc. | Use GET, HEAD, or OPTIONS |
| 413 | `Request headers too large` | Request headers exceed 8 KiB | Reduce header size (e.g., cookies, custom headers) |
| 416 | `Range not satisfiable` | Range header requests bytes outside file bounds | Use `HEAD` to get `Content-Length` first, then request a valid range |
| 500 | `Cannot open file` | Disk I/O error opening the file | Check file permissions and disk health |
| 503 | *(no JSON body)* | Maximum connections reached | Wait and retry; increase `http_max_connections` if this happens often |

---

## Troubleshooting

### Cannot connect to the HTTP server

1. **Verify Core is running** — check for `DG-LAN.Core.exe` in Task Manager.
2. **Check the port** — default is 59480, not 8080.
3. **Check firewall** — Windows Firewall may be blocking the port.
4. **Check logs** — Core logs `HttpServer listening on port 59480` on successful startup, or an error message if the port is in use.

### File returns 404 but it's in the shared folder

1. **URL encoding** — spaces must be `%20`: `My File.txt` → `My%20File.txt`
2. **Correct hash** — use `GET /api/v1/files` to get the current `shared_entry_hash` for each entry.
3. **Index not ready** — if you just added a shared folder, wait for Core to finish scanning and hashing.
4. **Peer redirect** — if the file is only on another peer and that peer's HTTP server is disabled, you'll get 404. Check that the peer has `http_server_enabled = true`.

### Video won't seek / keeps buffering

- Ensure the `Accept-Ranges: bytes` header is present in the response (it should be).
- If using a reverse proxy, make sure it isn't stripping the `Range` header or buffering the response. In nginx, set `proxy_buffering off;`.

### 503 Service Unavailable

The server has hit its connection limit (default 50). This can happen under heavy load. Options:
- Wait for existing connections to finish.
- Increase `http_max_connections` in the Core settings.
- Distribute load across multiple peers (each peer has its own HTTP server).

### How do I find the shared_entry_hash?

Call the file listing endpoint:

```bash
curl http://localhost:59480/api/v1/files | python -m json.tool
```

Each entry's `shared_entry_hash` field is the hex value to use in `/files/{hash}/{path}` URLs.
The `http_url` field contains a pre-built relative URL for convenience.

---

## Comparison: Built-in Server vs Python Bridge

DG-LAN has two HTTP serving systems. Choose the right one for your use case:

| Feature | Built-in C++ Server | Python dglan-api |
|---------|-------------------|-----------------|
| **Runs in** | Core process (automatic) | Separate Python process |
| **Default port** | 59480 | 8080 |
| **Dependencies** | None (compiled into Core) | Python 3.10+, protobuf |
| **File streaming** | Yes (`/files/{hash}/{path}`) | Yes (`/api/v1/files/{hash}/{path}`) |
| **`dglan://` links** | No | Yes (generates download links) |
| **Peer redirect** | Yes (302 to other peers) | No |
| **Peer discovery** | Yes (broadcasts HTTP port via IMAlive) | No |
| **Connection to Core** | Direct in-process access | TCP protobuf on port 59485 |
| **CORS** | Hardcoded `*` (all origins) | Configurable via `--cors-origins` |
| **Range support** | Yes | Yes |
| **ETag** | MD5-based (16 hex chars) | SHA3-prefix + mtime + size |
| **Content-Disposition** | Always `inline` | Configurable (`?download=1`) |
| **Max connections** | Configurable (default 50) | No limit (async) |
| **Authentication** | None | Core password support |
| **Best for** | LAN file access, video streaming, browser downloads | Website integration, `dglan://` link generation |

**Recommendation:**
- Use the **built-in server** for direct file access from browsers on the LAN.
- Use the **Python bridge** when you need `dglan://` download links for a website, configurable CORS, or forced download mode.
- Both can run simultaneously — they use different ports.

---

*See also: [dglan-api/README.md](dglan-api/README.md) for the Python bridge documentation, [dglan-api/HTTP-STREAMING.md](dglan-api/HTTP-STREAMING.md) for detailed Python streaming docs.*
