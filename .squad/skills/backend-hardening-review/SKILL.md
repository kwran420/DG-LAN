# Backend Hardening Review

## When to use

Use this skill when reviewing DG-LAN backend safety, modularisation seams, or test readiness across Core services and the Python bridge.

## Checklist

1. Map the live service graph from `application/Core/Core.cpp` and note every constructor dependency that crosses subsystem boundaries.
2. Compare the module list under `application/Core/` with the actual test projects plus `application/4.run_all_tests.sh`; treat missing suites and disabled entries as first-class risk, not bookkeeping.
3. Review both HTTP surfaces together: `application/Core/HttpServer/priv/HttpConnection.cpp` and `dglan-api/server.py`, then call out duplicated protocol/HTTP behavior, drift, and which side should stay authoritative.
4. Separate **strong evidence** from **environment blockers** by always recording what was validated in-workspace versus what is blocked by missing Qt/qmake/protoc or platform-specific runtime requirements.
5. Prefer recommendations that first isolate protocol adapters and global state (`SETTINGS`, large message handlers, direct cross-service wiring) before deeper modernization.
6. For pruning candidates, require both zero in-repo references and a live-code replacement or superseding path before deleting anything; if either proof is missing, leave it documented instead of pruning speculatively.

## Output expectations

- Cite concrete files and lines.
- Call out highest-risk ownership problems first.
- End with an ordered hardening plan: tests/evaluation, seams for extraction, modernization candidates, then pruning targets.
