# DG-LAN API Server

Standalone HTTP bridge that connects to a running DG-LAN Core and serves
shared file data as JSON — so your website can build `dglan://` download links.

## How it works

```
┌─────────────┐  TCP:59485  ┌────────────┐  HTTP:8080  ┌────────────┐
│  DG-LAN     │◄───────────►│  dglan-api │◄───────────►│  Website   │
│  Core       │  protobuf   │  server.py │    JSON     │  (JS)      │
└─────────────┘             └────────────┘             └────────────┘
```

1. `server.py` connects to Core on `127.0.0.1:59485` (same machine)
2. Core auto-authenticates local connections (no password needed locally)
3. Server receives the peer ID and browses all shared files
4. Website fetches `GET /api/files` → JSON with filenames, sizes, hashes, and
   pre-built `dglan://` URLs ready to use as `<a href="...">` links

## Quick start

```bash
# Install dependency
pip install protobuf

# Run (Core must be running on this machine)
python server.py

# Or with options
python server.py --http-port 9090 --http-host 0.0.0.0 --cors-origins https://mysite.com

# Remote Core — prefer environment variable for password
DGLAN_PASSWORD=mysecret python server.py --core-host 10.0.0.5
```

## API Endpoints

### `GET /api/v1/files`

Returns all shared files with `dglan://` link parameters.
Also available at `/api/files` for backward compatibility.

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
      "chunks": ["ff00aa...", "bb11cc...", ...],
      "dglan_url": "dglan://download?peer=0011aa...&hash=aabb11...&size=1073741824&name=movie.mkv&path=/"
    }
  ],
  "timestamp": 1712345678
}
```

### `GET /api/v1/status`

Core connection status and indexing progress.
Also available at `/api/status`.

```json
{
  "connected": true,
  "peer": "0011aabbccdd...",
  "cache_status": "UP_TO_DATE",
  "cache_progress": 100.0,
  "shared_entries": 3,
  "peers_online": 5
}
```

### `GET /api/v1/health`

Simple health check: `{"status": "ok"}`
Also available at `/api/health`.

## Website integration

Include `dglan-api.js` in your page:

```html
<script src="dglan-api.js"></script>
<div id="files"></div>
<script>
  (async () => {
    const api = new DglanApi("https://your-server.com:8080");
    const data = await api.getFiles();
    const container = document.getElementById("files");

    for (const file of data.files) {
      const row = document.createElement("div");
      row.className = "file-row";
      const link = document.createElement("a");
      link.href = file.dglan_url;
      link.textContent = file.name;
      const size = document.createElement("span");
      size.textContent = DglanApi.formatSize(file.size);
      row.appendChild(link);
      row.appendChild(size);
      container.appendChild(row);
    }
  })();
</script>
```

## Command-line options

| Flag | Default | Description |
|------|---------|-------------|
| `--core-host` | `127.0.0.1` | Core daemon IP |
| `--core-port` | `59485` | Core remote control port |
| `--password` | *(empty)* | Core remote password (prefer `DGLAN_PASSWORD` env var) |
| `--http-host` | `127.0.0.1` | HTTP listen address (use `0.0.0.0` for all interfaces) |
| `--http-port` | `8080` | HTTP listen port |
| `--cors-origins` | *(none)* | Allowed CORS origins (e.g., `https://mysite.com`) |
| `--verbose` | off | Debug logging |

## Deployment

Run this on the same machine as your DG-LAN Core. For production,
put it behind a reverse proxy (nginx/Caddy) with HTTPS:

```nginx
# nginx example
server {
    listen 443 ssl;
    server_name files.mysite.com;

    location /api/ {
        proxy_pass http://127.0.0.1:8080;
    }
}
```

## Requirements

- Python 3.10+
- `protobuf` package
- DG-LAN Core running on the same (or reachable) machine
