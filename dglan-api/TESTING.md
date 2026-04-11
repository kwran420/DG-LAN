# DG-LAN API — Testing Guide

## What is this?

A live HTTP API that connects to a running DG-LAN Core and serves shared file listings as JSON.
Use it to verify that `dglan://` download links work end-to-end.

---

## Setup

1. Start the DG-LAN Core on your machine.
2. Run the API server:
   ```bash
   pip install protobuf
   python server.py
   ```
3. The server listens on `http://127.0.0.1:8080` by default.

---

## Endpoints

### 1. Health check

```
GET /api/health
```

Expected: `{"status": "ok"}`

### 2. Status

```
GET /api/status
```

Shows Core connection state, indexing progress, and peer count.

### 3. File listing

```
GET /api/files
```

Returns JSON with every shared file. Each file includes:

| Field | Description |
|-------|-------------|
| `name` | Filename |
| `path` | Path inside the shared directory |
| `size` | File size in bytes |
| `peer` | 56-char hex peer ID |
| `shared_entry_hash` | 56-char hex shared entry ID |
| `chunks` | Array of chunk hashes |
| `dglan_url` | Ready-to-use `dglan://` download link |

---

## Testing a Download

1. **DG-LAN must be installed** on your machine with the `dglan://` URL scheme registered
   (the installer handles this, or see [BUILD.md](../BUILD.md#url-scheme-registration-dglan) for manual setup).

2. **Both machines must be on the same network** (LAN or ZeroTier) and see each other as peers in the DG-LAN GUI.

3. **Open the file listing** in your browser:
   ```
   http://<server-ip>:8080/api/files
   ```

4. **Copy any `dglan_url` value** from the JSON and paste it into your browser address bar:
   ```
   dglan://download?peer=3e4ad326...&hash=b3b1e1e4...&size=1626911&name=example.zip&path=%2F
   ```

5. **DG-LAN should launch** (or receive via IPC if already running) and start downloading the file.
