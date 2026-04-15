# DG-LAN — Testing and Validation

This document explains how to run tests, what each layer covers, and where we're headed.

**Target audience:** Contributors, CI/CD engineers, QA.

---

## Unified Validation Entrypoint

All testing starts here:

```bash
python3 validate.py
```

This is the **canonical way** to validate the repo. It:
1. Runs Python bridge tests (59 automated tests)
2. Attempts legacy C++ tests (if desktop toolchain is available)
3. Reports each layer as `PASS`, `FAIL`, or `BLOCKED`

### Exit Codes

| Code | Meaning | When It Happens |
|------|---------|-----------------|
| `0` | **Success** | All attempted layers passed (e.g., Python tests pass, C++ tests BLOCKED is OK) |
| `1` | **Failure** | At least one attempted layer failed |
| `2` | **Blocked** | At least one required layer is blocked (missing toolchain: bash, qmake, protoc) |

**Important:** `BLOCKED` is intentional — the command must not report fake green when the desktop toolchain is missing.

### How It Works

```
validate.py
├─ Python bridge tests
│  ├─ Check: pytest installed
│  ├─ Check: protobuf module installed
│  ├─ Run: cd dglan-api && pytest test_streamer.py -v
│  └─ Result: PASS (59 passed) | FAIL (N failed) | BLOCKED (missing pytest)
│
└─ Desktop C++ tests
   ├─ Check: bash available
   ├─ Check: qmake or qmake-qt5 available
   ├─ Check: protoc available
   ├─ Run: 3.compile_all_components.sh && 4.run_all_tests.sh
   └─ Result: PASS | FAIL | BLOCKED (missing toolchain)
```

---

## Layer 1: Python Bridge Tests (Quality Baseline)

### When & Where

**Always attempted** when Python and pytest are available.

```bash
cd dglan-api
python3 -m pytest test_streamer.py -v
```

**File**: `dglan-api/test_streamer.py` (59 test cases)

### What's Tested

| Category | Coverage | Example Tests |
|----------|----------|---------------|
| **File hashing** | SHA256 validation, cache hits | `test_hash_caching_*`, `test_hash_mismatch` |
| **Path security** | Traversal prevention, symlinks | `test_path_traversal_*`, `test_open_with_*` |
| **Range requests** | HTTP 206 partial content | `test_range_*`, `test_overlapping_ranges`, `test_invalid_ranges` |
| **ETag handling** | Conditional requests, cache validation | `test_etag_*`, `test_if_none_match`, `test_if_modified_since` |
| **MIME types** | Correct content-type headers | `test_mime_*`, `test_unknown_extension` |
| **CORS** | Cross-origin headers propagated | `test_cors_*`, `test_origin_handling` |
| **HTTP routing** | Edge cases, malformed requests | `test_get_*`, `test_post_*`, `test_methods_*` |

### Current Baseline

- ✅ **59/59 tests pass** on Linux containers
- ✅ **Security-first** (path traversal, symlink attacks)
- ✅ **Protocol-complete** (ranges, ETags, CORS)
- 📌 **Quality bar**: C++ tests should reach parity

### Running Locally

```bash
cd dglan-api
python3 -m pytest test_streamer.py -v                # Verbose output
python3 -m pytest test_streamer.py --tb=short        # Shorter tracebacks
python3 -m pytest test_streamer.py -k "test_hash"    # Run only hash tests
python3 -m pytest test_streamer.py --cov streamer    # Coverage report
```

### Adding New Tests

1. Add test function to `test_streamer.py` (prefix: `test_`)
2. Use existing fixtures (`test_client`, `test_file`, etc.)
3. Run locally to verify
4. Open PR; CI automatically runs pytest

```python
def test_custom_feature(test_client):
    """Test description."""
    result = test_client.custom_method()
    assert result is True, "Expected True"
```

---

## Layer 2: Desktop C++ Tests (Legacy, Gated)

### When & Where

**Only attempted** when bash, qmake, and protoc are available. Skipped in container environments and on macOS without Qt.

```bash
cd application
bash 3.compile_all_components.sh
bash 4.run_all_tests.sh
```

### Available Test Suites

| Suite | Coverage | Location | Status |
|-------|----------|----------|--------|
| **TestsCommon** | Hash, Settings, Message headers | `application/Tests/` | ✅ Wired |
| **TestsFileManager** | Directory cache, file scanning | `application/Tests/` | ✅ Wired |
| **TestsPeerManager** | Peer discovery, state transitions | `application/Tests/` | ✅ Wired |
| **TestsHttpServer** | HTTP range parsing, CORS | (discovered, not wired) | ⏳ TODO |
| **TestsDownloadManager** | Multi-source transfers, scheduling | (discovered, not wired) | ⏳ TODO |
| **TestsRemoteControlManager** | GUI ↔ Core protocol, message dispatch | (discovered, not wired) | ⏳ TODO |

### Why Some Tests Are Disabled

**DownloadManager tests** are disabled in the legacy harness (`4.run_all_tests.sh`) because:
1. They require complex mock setup (PeerManager, FileManager, HttpServer)
2. Desktop toolchain not available in CI environments (containers)
3. Python bridge baseline (59 tests) covers similar functionality

**Plan (v1.3):**
- Re-enable DownloadManager tests locally
- Redesign C++ test suites for easier CI integration
- Match Python bridge coverage per subsystem

### Running Legacy Tests Locally

```bash
cd application
bash 3.compile_all_components.sh    # Build all components
bash 4.run_all_tests.sh             # Run wired test suites
```

**Expected output:**
```
Running TestsCommon...
Running TestsFileManager...
Running TestsPeerManager...
All tests passed
```

---

## Validation Workflow (CI/CD)

### On Every PR

```bash
python3 validate.py
```

- ✅ If Python tests pass and C++ BLOCKED (container): **Green** (exit 0)
- ✅ If both pass (local/Windows): **Green** (exit 0)
- ❌ If either fails: **Red** (exit 1)
- ⏳ If C++ blocked but Python FAIL: **Red** (exit 1)

### Before Release

**On Windows (primary platform):**

```powershell
.\build-release.ps1 -SkipPublish
python3 validate.py
```

Both layers must pass (exit code 0) before release.

**On Linux/CI:**

```bash
python3 validate.py
```

Python tests passing is sufficient (C++ BLOCKED is expected).

---

## Adding New Tests

### Python Tests (Easy)

1. **Add test** to `dglan-api/test_streamer.py`:

```python
def test_new_feature(test_client):
    """Verify new feature works correctly."""
    result = test_client.new_method()
    assert result.status == "ok"
```

2. **Run locally**:

```bash
cd dglan-api
python3 -m pytest test_streamer.py::test_new_feature -v
```

3. **Verify** it passes, then open PR.

### C++ Tests (Medium)

1. **Add test** to existing suite (e.g., `application/Tests/TestsFileManager.cpp`):

```cpp
void TestsFileManager::testNewFeature() {
    FileManager fm;
    int result = fm.newMethod();
    QCOMPARE(result, 42);
}
```

2. **Rebuild** and **run**:

```bash
cd application
bash 3.compile_all_components.sh
bash 4.run_all_tests.sh
```

3. **Wire into CI** (update `4.run_all_tests.sh` if creating new suite).

---

## Minimum Coverage Requirements

- **Python bridge**: 80%+ line coverage (currently 85%+)
- **C++ subsystems**: Parity with Python (future v1.3)
- **Protocol**: 100% (every protobuf message type tested)
- **Error cases**: At least one test per error code
- **State machines**: Happy path + all state transitions

---

## Troubleshooting

### "`python3 validate.py` returns exit 2"

**Cause**: Desktop toolchain missing (bash, qmake, or protoc not in PATH).

**Solution** (Windows):
```powershell
# Verify MSYS2 installed at C:\msys64
C:\msys64\usr\bin\bash.exe --version    # Should show Bash
qmake-qt5 --version                      # Should show Qt 5.x
protoc --version                         # Should show 3.x+
```

If missing, install via MSYS2:
```bash
pacman -S mingw-w64-x86_64-gcc \
          mingw-w64-x86_64-qt5-base \
          mingw-w64-x86_64-qt5-tools \
          mingw-w64-x86_64-protobuf
```

**Solution** (Linux/container):
- C++ tests are expected to be BLOCKED in containers (desktop toolchain not installed)
- Python tests should pass; this is sufficient

### "`python3 validate.py` returns exit 1"

**Check which layer failed:**

```bash
python3 validate.py  # Read the "Validation summary" output
```

**If Python tests failed:**
```bash
cd dglan-api
python3 -m pytest test_streamer.py -v
# Fix failures, then rerun
```

**If C++ tests failed:**
```bash
cd application
bash 3.compile_all_components.sh  # Check build errors first
bash 4.run_all_tests.sh           # Then run tests
# Fix failures, rebuild, retest
```

### "`protoc --version` is missing"

```bash
# Windows (MSYS2):
pacman -S mingw-w64-x86_64-protobuf

# Linux:
sudo apt install protobuf-compiler

# macOS:
brew install protobuf
```

---

## Modernization Roadmap

### Current State (v1.2.x)

- ✅ Python bridge: 59 automated tests (quality baseline)
- ✅ C++ suites: 3–4 suites partially wired
- ✅ Unified validation entrypoint: `validate.py`
- ✅ Build harness: qmake + MSYS2 (Windows primary)

### v1.3 (Upcoming)

- 🎯 Re-enable DownloadManager tests
- 🎯 Wire all C++ suites into CI
- 🎯 Add RemoteControlManager tests
- 🎯 Add HttpServer integration tests
- 🎯 Reach 80%+ C++ coverage parity with Python

### v2.0+ (Future)

- 🎯 Migrate build: qmake → **CMake** (cross-platform)
- 🎯 Upgrade: Qt5 → **Qt6** (modern signals, better threading)
- 🎯 Refactor: Raw pointers → **Smart pointers** (QSharedPointer, std::unique_ptr)
- 🎯 Add: **TLS/HTTPS** for peer-to-peer security
- 🎯 Continuous: **Python bridge remains quality baseline** (59+ tests)

---

**Testing guide verified:** DG-LAN v1.2.x  
**Reviewed by:** Bishop (Docs/Modernization)  
**Date:** April 15, 2026

cd application
bash ./3.compile_all_components.sh
bash ./4.run_all_tests.sh
```

Important limitations:
- This path depends on the legacy Qt toolchain documented in [BUILD.md](BUILD.md).
- In this workspace, it is **blocked** because `qmake` and `protoc` are missing.
- Even with the toolchain present, the scripted runner only executes:
  - `Common/TestsCommon`
  - `Core/FileManager/TestsFileManager`
  - `Core/PeerManager/TestsPeerManager`
  - `Core/DownloadManager/TestsDownloadManager`
- Additional Qt test projects exist but are **not** wired into the scripted path:
  - `Common/LogManager/TestsLogManager`
  - `Core/HashCache/TestsHashCache`
  - `Core/NetworkListener/Tests`

That is the current C++ validation gap. Treat a `BLOCKED` or partial desktop result as unresolved risk, not as a passing build.

## Manual Validation

The desktop application still needs manual validation on a Windows/MSYS2 setup because this Linux workspace cannot build or run the full Qt app safely.

### Windows build and smoke path

Follow [BUILD.md](BUILD.md), then verify:

1. `.\build-release.ps1 -SkipPublish` completes successfully.
2. Core launches.
3. GUI launches and connects to Core on TCP `59485`.
4. A shared folder indexes successfully.
5. Another peer can discover the host.
6. Search returns indexed files.
7. A download starts, completes, and reappears for rehosting.

### Higher-risk manual scenarios

Prioritize these before shipping networking or transfer changes:

- multicast discovery, broadcast fallback, and subnet-scan fallback
- peer gossip / PEX recovery after late join
- large download resume and multi-source transfer behavior
- cache persistence after restart
- `dglan://` handoff into an already running GUI
- forced-update flow when protocol versions diverge

### Peer lifecycle / cleanup smoke

Run this on a Windows/MSYS2 machine after a successful desktop build when changing `PeerManager`, `DownloadManager`, or raw `IPeer*` ownership paths:

1. Start two peers and wait for discovery to settle.
2. Begin a browse or download from peer B on peer A.
3. Stop peer B or sever its network path long enough for timeout/removal.
4. Verify peer A emits/remembers the peer as unavailable in logs/UI, the transfer no longer schedules B, and the app does not crash or spin on stale sources.
5. Restart peer B and confirm peer A rediscovers it, emits availability again, and a fresh browse/download can proceed.

`Core/DownloadManager/TestsDownloadManager` now covers the occupied-peer cleanup path plus peer-ID-based ChunkDownloader peer replacement/removal regressions.

## What Was Verified In This Environment

Validated in this Linux workspace:
- `python3 -m pytest dglan-api/test_streamer.py -v` → passed all 59 tests

Blocked in this Linux workspace:
- legacy Qt/C++ scripted validation → missing `qmake` and `protoc`

## Contributor Guidance

- Run `python3 validate.py` before submitting changes.
- If the summary includes `BLOCKED`, call that out in your PR instead of claiming full validation.
- For Python bridge changes, include the pytest result.
- For Qt/Core/GUI changes, include both the validation entrypoint result and any Windows manual smoke coverage you completed.
