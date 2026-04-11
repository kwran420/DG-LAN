# DG-LAN Python API Bridge

Standalone HTTP server that connects to a running DG-LAN Core and serves
shared files over HTTP — with `dglan://` download links, configurable CORS,
and forced-download mode.

> **Not sure which HTTP server to use?** DG-LAN also has a [built-in HTTP server](../HTTP-SERVER.md)
> compiled directly into Core (port 59480, zero dependencies). This Python bridge
> adds `dglan://` links, configurable CORS, and `?download=1` support.
> See the [comparison table](../HTTP-SERVER.md#comparison-built-in-server-vs-python-bridge).

## Quick Start

```bash
pip install protobuf
python server.py                        # Core must be running
python server.py --cors-origins "*"     # Allow cross-origin requests
DGLAN_PASSWORD=secret python server.py --core-host 10.0.0.5  # Remote Core
```

Server starts on `http://127.0.0.1:8080`.

## API Endpoints

| Endpoint | Description |
|----------|-------------|
| `GET /api/v1/files` | File listing with `dglan://` and HTTP streaming URLs |
| `GET /api/v1/files/{hash}/{path}` | Stream a file (Range, ETag, `?download=1`) |
| `GET /api/v1/status` | Core connection status and peer count |
| `GET /api/v1/health` | Liveness probe: `{"status": "ok"}` |

All `/api/v1/` endpoints have backward-compatible aliases at `/api/` (e.g., `/api/files`).

## Configuration

| Flag | Default | Description |
|------|---------|-------------|
| `--core-host` | `127.0.0.1` | Core daemon IP |
| `--core-port` | `59485` | Core remote control port |
| `--password` | *(empty)* | Core password (prefer `DGLAN_PASSWORD` env var) |
| `--http-host` | `127.0.0.1` | HTTP listen address (`0.0.0.0` for all interfaces) |
| `--http-port` | `8080` | HTTP listen port |
| `--cors-origins` | *(none)* | Allowed CORS origins (e.g., `https://mysite.com`) |
| `--verbose` / `-v` | off | Debug logging |

## Requirements

- Python 3.10+
- `protobuf` package
- DG-LAN Core running on the same (or reachable) machine

## Documentation

| Document | Description |
|----------|-------------|
| [HTTP-STREAMING.md](HTTP-STREAMING.md) | Full API reference, HTTP features, JavaScript integration, security, deployment |
| [TESTING.md](TESTING.md) | Testing guide — manual tests and 59-test automated suite |
| [Built-in HTTP Server](../HTTP-SERVER.md) | C++ HTTP server embedded in Core (port 59480) |
