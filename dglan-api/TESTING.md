# DG-LAN API — Testing Guide

## What is this?

A live HTTP API running on my machine that serves the list of shared files from my DG-LAN Core.
You can use it to verify the `dglan://` download links work end-to-end.

---

## Server Address

```
http://10.11.245.80:8080
```

> You must be on the same ZeroTier network to reach this IP.

---

## Endpoints to try

### 1. Health check — is it up?

```
http://10.11.245.80:8080/api/health
```

Expected response:
```json
{"status": "ok"}
```

### 2. Status — Core connection info

```
http://10.11.245.80:8080/api/status
```

Shows whether the Core is connected, indexing progress, number of peers online, etc.

### 3. File listing — the main one

```
http://10.11.245.80:8080/api/files
```

Returns JSON with every shared file. Each file includes:

| Field | What it is |
|-------|-----------|
| `name` | Filename |
| `path` | Path inside the shared directory |
| `size` | File size in bytes |
| `peer` | 56-char hex peer ID |
| `shared_entry_hash` | 56-char hex shared entry ID |
| `chunks` | Array of chunk hashes |
| `dglan_url` | Ready-to-click `dglan://` download link |

---

## How to test a download

1. **Make sure DG-LAN is installed** on your machine and the `dglan://` URL scheme is registered
   (see the installer or run the registry commands from the README).

2. **Make sure you're on the same ZeroTier network** and can see my machine as a peer in the DG-LAN GUI.

3. **Open the file listing** in your browser:
   ```
   http://10.11.245.80:8080/api/files
   ```

4. **Copy any `dglan_url` value** from the JSON and paste it into your browser address bar.
   It will look like:
   ```
   dglan://download?peer=3e4ad326...&hash=b3b1e1e4...&size=1626911&name=7zip.zip&path=%2F
   ```

5. **DG-LAN should launch** (or receive via IPC if already running) and start downloading the file.

---

## Quick test from the command line

### PowerShell
```powershell
Invoke-RestMethod http://10.11.245.80:8080/api/status
Invoke-RestMethod http://10.11.245.80:8080/api/files | ConvertTo-Json -Depth 5
```

### curl
```bash
curl http://10.11.245.80:8080/api/files | jq '.files[0]'
```

### Python
```python
import urllib.request, json
r = urllib.request.urlopen("http://10.11.245.80:8080/api/files")
data = json.loads(r.read())
for f in data["files"][:5]:
    print(f"{f['name']}  ({f['size']} bytes)")
    print(f"  {f['dglan_url']}")
```

---

## What to report back

- [ ] Can you reach `/api/health`?
- [ ] Does `/api/files` return the file list?
- [ ] Does clicking/pasting a `dglan_url` trigger DG-LAN?
- [ ] Does the download actually start and complete?
- [ ] Any errors or weird behaviour?

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Can't connect to `10.11.245.80:8080` | Check you're on the ZeroTier network. Try `ping 10.11.245.80`. |
| `dglan://` link doesn't open anything | DG-LAN isn't installed or the URL scheme isn't registered. Run the `reg add` commands from the README. |
| Download starts but peer not found | Make sure DG-LAN Core is running on both machines and they can see each other in the peer list. |
| API returns `{"files": []}` | My Core might be restarting or re-indexing. Try `/api/status` to check `cache_status`. |
