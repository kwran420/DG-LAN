# DG-LAN — Code Style Guide

Guidelines for C++17, Qt5, and Python contributions to DG-LAN.

**Target audience:** Contributors writing Core, GUI, Common, and Test code.

---

## C++17 Style

### Naming Conventions

| Element | Convention | Example |
|---------|-----------|---------|
| **Classes** | CamelCase, noun-based | `FileManager`, `PeerManager`, `HttpServer`, `RemoteConnection` |
| **Methods** | camelCase, verb-based | `getEntries()`, `getFileHash()`, `addPeer()`, `removePeer()` |
| **Member variables (public)** | camelCase | `currentStatus`, `downloadSpeed` |
| **Member variables (private)** | `m_name` (Qt convention) | `m_fileCache`, `m_peers`, `m_mutex` |
| **Constants & macros** | `SCREAMING_SNAKE_CASE` | `MAX_PEERS`, `DEFAULT_TIMEOUT_MS`, `PROTOCOL_VERSION` |
| **Enums** | CamelCase, singular noun | `PeerStatus`, `ConnectionState` |
| **Enum values** | CamelCase, match purpose | `PeerStatus::Online`, `ConnectionState::Connecting` |

### Header Guards & Includes

- **Use `#pragma once`** (modern, simpler, supported by all compilers in use)
- Place includes in order: stdlib → Qt → local (each group sorted alphabetically, blank lines between)

```cpp
// FileManager.h
#pragma once

#include <vector>
#include <unordered_map>

#include <QObject>
#include <QString>
#include <QThread>

#include "Common/Hash.h"
#include "Common/Settings.h"
```

### Function Design

- **Length**: Keep functions under 50 lines; split complex logic
- **Nesting**: Max 3 levels deep; use early returns to reduce nesting
- **Single responsibility**: Each function does one thing well
- **Error handling**: Return error codes for expected failure cases; throw std::runtime_error for invariant violations

```cpp
// Good: Early returns, clear flow
int FileManager::addPeer(const Peer* peer) {
    if (!peer) return ErrorCode::InvalidPeer;
    if (peers.count(peer->id)) return ErrorCode::PeerAlreadyAdded;
    
    peers[peer->id] = peer;
    return ErrorCode::Success;
}

// Bad: Deeply nested, hard to follow
if (peer) {
    if (peers.count(peer->id) == 0) {
        peers[peer->id] = peer;
        return ErrorCode::Success;
    }
    return ErrorCode::PeerAlreadyAdded;
}
```

### Comment Style

- **Document the "why"**, not the "what" (code shows the what)
- Use `//` for single-line comments, `/* */` for multi-line blocks
- No commented-out code — delete it and rely on git history
- Comments for non-obvious logic, invariants, and threading/locking

```cpp
// Good: Explains the non-obvious
// We probe port sequentially starting from the configured port; this avoids
// stale socket TIME_WAIT collisions on service restart
int portToTry = configuredPort;
for (int i = 0; i < 10; ++i) {
    if (tryBind(portToTry)) return portToTry;
    portToTry++;
}

// Bad: Comments obvious code
int x = a + b;  // Add a and b together
```

### Thread Safety & Concurrency

- **Protect shared state** with `QMutex` (or `std::mutex` if no Qt event loop)
- **Signal/slot pattern** for async operations (prefer signals over callbacks)
- **Never hold locks across wait points** — deadlock risk
- **Scope guards**: Use RAII for lock management

```cpp
// Good: Qt signal/slot for async cleanup
connect(this, &Peer::becameDead, manager, &DownloadManager::removePeerDownloads);

// Good: Scoped lock
{
    QMutexLocker locker(&m_cacheMutex);
    m_cache->addEntry(entry);
}  // Lock released here
```

### Qt Patterns

#### Signals & Slots

- **Emit signals** for state changes (peer joined, download started, file indexed)
- **Never hold locks** in slots that emit signals (risk of deadlock)
- **Use `Q_EMIT`** macro (deprecated in Qt6, but standard in Qt5)

```cpp
// In .h
signals:
    void fileIndexed(const QString &path);
    void peerBecameUnavailable(const Common::Hash &peerId);

// In .cpp
Q_EMIT fileIndexed(entry.path);
Q_EMIT peerBecameUnavailable(peerId);
```

#### Memory Management

- **Prefer QObject parent/child** for automatic cleanup (GUI, settings)
- **Use QSharedPointer** for long-lived objects shared across threads
- **Avoid raw pointers** in public interfaces (use Hash IDs or shared pointers)
- **QWeakPointer** when you need a non-owning reference that can detect deletion

```cpp
// Good: Parent-based cleanup
auto widget = new FileListWidget(parentWidget);  // ~FileListWidget when parent deleted

// Good: Hash-based peer references (avoids raw pointer lifetime issues)
struct DownloadState {
    Common::Hash peerId;  // Instead of Peer*
    bool hasChunk(int i);
};

// Good: Signal cleanup for peer death
connect(peerManager, &PeerManager::peerBecamesUnavailable,
        downloadManager, &DownloadManager::removePeerDownloads);
```

### Error Codes

Define error codes as enums in a common header (e.g., `Common/ErrorCode.h`):

```cpp
// Common/ErrorCode.h
enum class ErrorCode {
    Success = 0,
    InvalidPeer = 1,
    PeerAlreadyAdded = 2,
    FileNotFound = 3,
    DownloadFailed = 4,
};
```

Return by value, not via output parameters:
```cpp
// Good
ErrorCode addPeer(const Peer* peer);

// Less good (output parameter hard to spot at call site)
void addPeer(const Peer* peer, ErrorCode& result);
```

---

## Qt GUI Code (Core ↔ GUI Protocol)

### Message Passing

- **Use protobuf** for serialization (defined in `Protos/`)
- **Wrap in `Common::MessageHeader`** with type code for dispatch
- **Async requests/responses**: Signal when ready, not blocking calls

```cpp
// Core sends file list to GUI
{
    QMutexLocker locker(&m_fileCacheMutex);
    gui_protocol::FileListUpdate msg = serializeFileList();
    auto header = Common::MessageHeader::create(MessageType::FileListUpdate, msg);
    socket->write(header.serialize());
}
```

### Widget Design

- **One widget per responsibility** (NetworkWidget, PeersDock, LogDock)
- **QAbstractItemModel** for data (LogModel, PeerModel)
- **Slots for state updates**, signals for user actions
- **Defer heavy work** to background threads (QThread + slots, not blocking main)

---

## Python Bridge & API Code

### Style

- **PEP 8** for file layout and naming
- **Type hints** for all functions (Python 3.9+ style)
- **Docstrings** for classes and public methods (Google-style)

```python
def validate_hash(file_path: Path, expected_hash: str) -> bool:
    """Validate file integrity against expected hash.
    
    Args:
        file_path: Full path to file.
        expected_hash: Hex-encoded SHA256.
    
    Returns:
        True if hash matches, False otherwise.
    """
    computed = hashlib.sha256(file_path.read_bytes()).hexdigest()
    return computed == expected_hash
```

### Error Handling

- **Raise exceptions** for invariant violations or unrecoverable errors
- **Return error results** for expected operational failures

```python
# Unrecoverable: raise
if not os.access(config_dir, os.W_OK):
    raise PermissionError(f"Cannot write to {config_dir}")

# Expected failure: return result
def get_file(self, path: str) -> Optional[bytes]:
    if not self.is_valid_path(path):
        return None
    return self.read_file(path)
```

---

## Logging

### Logging Levels (C++)

Define in `Common/Logging.h`:

| Level | When | Example |
|-------|------|---------|
| `L_DEBUG` | Detailed debugging info (verbose, dev-only usually) | Peer connect/disconnect, chunk fetches |
| `L_INFO` | Normal operation milestones | Service started, peer joined, file indexed |
| `L_WARN` | Recoverable issues | Peer timeout, slow transfer, retry attempt |
| `L_ERROR` | Failures requiring attention | Download failed, service crash, corrupt file |

Use sparingly; avoid spam. In Core:
```cpp
L_INFO << "Peer " << peerId << " joined";
L_WARN << "Download " << dlId << " exceeded timeout; retrying...";
L_ERROR << "Failed to bind HTTP port " << port << "; code " << errno;
```

### Logging Levels (Python)

```python
import logging

logger = logging.getLogger(__name__)
logger.debug("Parsed range request: bytes=%s-%s", start, end)
logger.info("Serving file: %s (%.1f MB)", path, size_mb)
logger.warning("Client disconnect during transfer: %s", path)
logger.error("Hash verification failed for %s", path)
```

---

## Testing

### C++ Tests (Qt/qmake-based)

- **File naming**: `Tests<Subsystem>.cpp` (e.g., `TestsFileManager.cpp`, `TestsHttpServer.cpp`)
- **Scope**: Test boundary conditions, error cases, state transitions
- **Baseline**: Python bridge has 59 tests; C++ should reach parity per subsystem

```cpp
// Tests should cover:
// 1. Happy path (normal operation)
// 2. Boundary conditions (empty, max size, null)
// 3. Error cases (invalid input, resource limits)
// 4. State machine transitions (connect→auth→ready, disconnect)
void TestsDownloadManager::testAddDownloadWithInvalidPeer() {
    DownloadManager dm;
    auto result = dm.addDownload("nonexistent-file", nullptr);
    QCOMPARE(result, ErrorCode::InvalidPeer);
}
```

### Python Tests (pytest)

- **File naming**: `test_<module>.py`
- **Fixture setup**: Use pytest fixtures for reusable state
- **Coverage**: Run with `pytest --cov` to verify 80%+ coverage

```python
import pytest

@pytest.fixture
def test_client(tmp_path):
    """Provide isolated test client."""
    return TestClient(config_dir=tmp_path)

def test_get_nonexistent_file(test_client):
    result = test_client.get_file("nonexistent")
    assert result is None
```

### Validation Entrypoint

Run all available tests via:
```bash
python3 validate.py
```

Exit codes:
- `0`: All tests pass
- `1`: At least one test failed
- `2`: At least one required toolchain is missing

---

## Common Anti-Patterns to Avoid

| Anti-Pattern | Problem | Solution |
|--------------|---------|----------|
| **Raw pointers in containers** | Stale pointers, crashes | Use Hash IDs or shared pointers |
| **Holding locks in signals** | Deadlock risk | Release lock before emitting |
| **Blocking calls on main thread** | UI freeze | Use QThread + signals |
| **Commented-out code** | Maintenance burden | Delete it; use git history if needed |
| **Magic numbers** | Hard to maintain | Extract into named constants |
| **Functions > 50 lines** | Hard to test, understand | Refactor into smaller functions |
| **No error handling** | Silent failures, crashes | Return error codes or throw |
| **Inconsistent naming** | Confusion, merge conflicts | Follow conventions above |

---

## Code Review Checklist

Before submitting a PR, verify:

- [ ] **Naming** follows CamelCase (classes), camelCase (methods), m_name (members)
- [ ] **Functions** are < 50 lines with max 3 nesting levels
- [ ] **Comments** explain "why", not "what"; no commented-out code
- [ ] **Qt code** uses signals/slots, not callbacks; proper parent/child
- [ ] **Thread safety** protected with QMutex; no locks held in signals
- [ ] **Error handling** returns error codes or throws; no silent failures
- [ ] **Tests** added for new logic; Python bridge tests all pass
- [ ] **Build** succeeds: `.\build-release.ps1 -SkipPublish`
- [ ] **Validation** passes: `python3 validate.py`

---

## Modernization Notes

### Current Stack (v1.2.x)

- **C++17** + **Qt5** + **qmake** (working, stable)
- **Protobuf 3** (pre-generated .pb.cc/.pb.h in Protos/)
- **MSYS2 MinGW64** on Windows (primary); Linux/macOS experimental
- **Python 3.9+** for bridge + validation

### Future Paths (v2.0+)

- **CMake** to replace qmake (cross-platform, easier dependency management)
- **Qt6** to replace Qt5 (modern signals, better performance)
- **Smart pointers** (QSharedPointer, std::unique_ptr) to replace raw pointers
- **TLS/HTTPS** for peer-to-peer security
- **C++20** features (coroutines, modules) if feasible

Until these are adopted, follow the style guide above to keep the codebase consistent and safe for future refactoring.

---

**Guide verified:** DG-LAN v1.2.x  
**Reviewed by:** Bishop (Docs/Modernization)  
**Date:** April 15, 2026
