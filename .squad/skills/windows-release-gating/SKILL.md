# Windows Release Gating

## Purpose

Use this pattern when DG-LAN release work is requested from a non-Windows environment.

## Pattern

1. Inspect git state first so you know whether the release commit already exists locally.
2. Push the branch before any release-script attempt if the requested work explicitly includes commit/push.
3. Run `python3 validate.py` to separate runnable validation from blocked desktop coverage.
4. Check for a usable PowerShell host plus Windows prerequisites (`C:\msys64`, MinGW Qt runtime, Inno Setup) before claiming `build-release.ps1` can run.
5. If those prerequisites are missing, report the release build as blocked rather than partially successful.

## DG-LAN Example

- Green validation in this Linux workspace: `python3 validate.py` → 59 Python tests passed, desktop layer blocked on `qmake`/`protoc`
- Blocking release prerequisites here: no `pwsh`/`powershell`, no Windows MSYS2/Inno Setup paths
- Safe script invocation when prerequisites exist: `pwsh -NoProfile -File ./build-release.ps1 -SkipPublish`
