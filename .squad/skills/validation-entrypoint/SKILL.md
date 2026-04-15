# Validation Entrypoint

## Purpose

Use this pattern when a legacy repo has some runnable validations plus one or more blocked platform-specific layers.

## Pattern

1. Add one small repo-root entrypoint that calls the existing validation commands without inventing a new framework.
2. Report each layer independently as:
   - `PASS`
   - `FAIL`
   - `BLOCKED`
3. Return non-zero when any required layer is blocked so contributors cannot confuse partial coverage with a fully green build.
4. Document the exact automated commands, manual checks, and known validation gaps in a project-level `TESTING.md`.

## DG-LAN Example

- Entry point: `python3 validate.py`
- Automated green layer: `cd dglan-api && python3 -m pytest test_streamer.py -v`
- Blocked layer: Qt/C++ desktop validation when `qmake` or `protoc` is unavailable
- Known scripted gap: legacy desktop scripts do not wire every discovered Qt test project into one runner
