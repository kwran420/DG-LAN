# Contributing to DG-LAN

Thanks for your interest in contributing! DG-LAN is a decentralized LAN file-sharing app built with C++17, Qt5, and Protobuf.

## Getting Started

1. **Fork** the repository on GitHub.
2. **Clone** your fork locally.
3. **Build** using the instructions in [BUILD.md](BUILD.md):
   ```powershell
   .\build-release.ps1 -SkipPublish
   ```
4. Make your changes on a feature branch.
5. **Test** your changes — run the app and verify the feature works.
6. Open a **Pull Request**.

## Development Setup

**Required toolchain (Windows):**
- MSYS2 with MinGW64 packages: gcc, qt5-base, qt5-tools, protobuf, openssl, make
- Inno Setup 6 (for installer builds)

See [BUILD.md](BUILD.md) for full setup details.

## Branch Naming

Use the format `type/short-description`:
- `feature/add-search-filter`
- `fix/download-queue-numbering`
- `docs/update-readme`

## Commit Messages

Follow [Conventional Commits](https://www.conventionalcommits.org/):

```
type(scope): short description

Optional longer explanation.
```

**Types:** `feat`, `fix`, `chore`, `docs`, `refactor`, `test`, `perf`, `ci`, `build`

**Examples:**
- `feat(gui): add download queue reorder buttons`
- `fix(core): prevent crash on empty peer list`
- `docs: update BUILD.md with MSYS2 instructions`

## Code Style

- **C++17** standard
- Meaningful, descriptive names from the problem domain
- Functions under 50 lines, single responsibility
- Early returns to reduce nesting (max 3 levels)
- No commented-out code, no leftover debug statements
- Magic numbers and strings extracted into named constants

## Pull Request Guidelines

Your PR should include:
- **What** changed
- **Why** it was changed
- **How** it was tested

All code changes should build without errors. Run `.\build-release.ps1 -SkipPublish` to verify before submitting.

## Reporting Bugs

Use the [Bug Report](https://github.com/kwran420/DG-LAN/issues/new?template=bug_report.md) issue template. Include:
- Steps to reproduce
- Expected vs actual behavior
- DG-LAN version and OS
- Log output if applicable

## Suggesting Features

Use the [Feature Request](https://github.com/kwran420/DG-LAN/issues/new?template=feature_request.md) issue template.

## License

By contributing, you agree that your contributions will be licensed under the [GPLv3](COPYING).
