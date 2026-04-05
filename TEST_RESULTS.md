# DG-LAN v1.2.0 Alpha — Test Results

**Date:** 5 April 2026  
**Build:** DG-LAN 1.2.0 Alpha (GCC 15.2.0, Qt 5.15.18, built 07:36 ACST)  
**Platform:** Windows 10/11 x64 (MSYS2 MinGW64 runtime)  
**Installer:** `DG-LAN-1.2.0Alpha-2026-04-05_07-36-Setup.exe`  
**Tester:** Automated (PowerShell + Win32 API + UIAutomation screenshots)

---

## Installation

| Item | Status | Notes |
|------|--------|-------|
| Installer builds without error | ✅ PASS | Inno Setup 6.7.1 |
| App launches after manual DLL copy | ✅ PASS | 14 DLLs + imageformat plugins + libpcre2-8-0.dll required |
| Title bar shows "DG-LAN" | ✅ PASS | Rebrand confirmed |

**DLLs required (bundled in installer):**  
libmd4c, libpng16-16, libharfbuzz-0, libfreetype-6, libbrotlidec, libbrotlicommon,  
libbz2-1, libglib-2.0-0, libgraphite2, libiconv-2, libintl-8, libjpeg-8,  
libssl-3-x64, libcrypto-3-x64, **libpcre2-8-0** (fix applied 5 Apr 2026)

---

## UI Functional Tests

### Main Window
| Feature | Status | Notes |
|---------|--------|-------|
| Window title "DG-LAN" | ✅ PASS | |
| Tab bar (6 tabs) | ✅ PASS | Settings \| Chat \| Downloads \| Uploads \| Activity \| Indexing |
| Peer discovery | ✅ PASS | DARWINGAMERS detected (LAN peer) |
| Status bar | ✅ PASS | Shows download/upload rates, peer count, core status |

### Tabs
| Tab | Status | Notes |
|-----|--------|-------|
| Settings | ✅ PASS | Sub-tabs: Basic, Network, User Interface, Core Seeders |
| Chat | ✅ PASS | Shows chat messages from peers |
| Downloads | ✅ PASS | Shows empty list (correct, no active downloads) |
| Uploads | ✅ PASS | Shows empty list (correct, no active uploads) |
| Activity | ✅ PASS | Shows "Activity Log" with hashing status entries |
| Indexing | ✅ PASS | Shows "Hashing — computing content fingerprints" at 0.0% |

### Dialogs
| Dialog | Status | Notes |
|--------|--------|-------|
| About DG-LAN | ✅ PASS | Shows v1.2.0 Alpha, "Plug in, game on!", build date, contributors |
| About — Version | ✅ PASS | DG-LAN 1.2.0 Alpha |
| About — Tagline | ✅ PASS | "Plug in, game on!" |
| About — Contributors | ✅ PASS | "DG-LAN contributors: Kieran Hollis & Matthew Dix — 2026" |
| About — Logo | ⚠️ COSM | Logo renders but is dark on dark background (cosmetic) |

### System Tray
| Feature | Status | Notes |
|---------|--------|-------|
| Tray icon shows | ✅ PASS | trayIcon.show() called, icon = DG-LAN game controller |
| Tray tooltip | ✅ PASS | "DG-LAN" tooltip set |
| Tray menu items | ✅ PASS | "Show DG-LAN", "Exit" |
| Tray left-click | ✅ PASS | Raises main window |

### Networking
| Feature | Status | Notes |
|---------|--------|-------|
| Peer discovery (LAN) | ✅ PASS | DARWINGAMERS peer found automatically |
| Core connection | ✅ PASS | Status bar: "Core: connected - hashing in progress..." |

---

## Known Issues

| ID | Severity | Description | Status |
|----|----------|-------------|--------|
| UI-01 | Low | About dialog logo barely visible (dark-on-dark) | Open / cosmetic |

---

## Fixes Applied This Session

| Fix | File | Description |
|-----|------|-------------|
| DLL bundling | `windows_setup.iss` | Added 14 missing Qt5 runtime DLLs |
| libpcre2-8-0 | `windows_setup.iss` | Removed `skipifsourcedoesntexist` flag |
| Qt image plugins | Install dir | Manually deployed: qjpeg, qpng, qsvg, qgif, qico |

---

## Build Information
- **Repository:** https://github.com/kwran420/DG-LAN
- **Commit:** `9c9df865` (installer fix: libpcre2-8-0.dll)
- **Previous:** `799cc2fd` (bundle missing Qt5 DLLs)
- **Installer output:** `application/Setups/Windows/Installations/DG-LAN-1.2.0Alpha-2026-04-05_07-36-Setup.exe`
