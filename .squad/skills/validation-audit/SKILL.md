# Validation Audit

## Purpose

Use this pattern when auditing a mixed legacy repo before refactors or modernization.

## Steps

1. Read the top-level build, test, and project-context docs first.
2. Locate the project-native validation entrypoints (scripts, test runners, release scripts).
3. Compare documented validation with actual files present in the repo.
4. Run only the repo's existing validation commands where feasible in the current environment.
5. Separate results into:
   - executable and passing
   - executable but failing
   - blocked by environment or missing build artifacts
6. Check whether any existing test projects are not wired into the main scripts.
7. Prioritize safety work in this order:
   - make one green automated entrypoint
   - cover highest-risk protocol/data flows
   - add release/install smoke checks
   - only then push large structural refactors

## DG-LAN example

- Python bridge: `cd dglan-api && pytest test_streamer.py -v`
- Legacy C++ path: `application/3.compile_all_components.sh` then `application/4.run_all_tests.sh`
- Common audit finding: docs can claim CI/build paths that are no longer present, so always verify the files and commands directly.
