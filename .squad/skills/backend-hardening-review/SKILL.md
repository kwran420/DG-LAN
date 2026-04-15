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

## Reusable pattern

- When a backend path still needs raw `IPeer*` execution handles, first move any long-lived indexes, occupancy sets, and scheduler bookkeeping to stable peer IDs (`Common::Hash`), then use `peerBecomesUnavailable` / `peerBecomesAvailable` only to clear or refresh the transient raw handles.
- In transition code that still keeps raw `IPeer*` lists (for example transfer workers), canonicalise inserts/removals by peer ID instead of pointer equality so a rediscovered peer instance refreshes the slot instead of duplicating or leaking stale handles.
- For DG-LAN HTTP surfaces, only claim behaviour the server can prove locally. If a request path cannot verify remote ownership, do not blind-redirect; instead expose a `dglan://` handoff (`launch_url` / native link) so browsers can enter the real chunk-balanced downloader without overstating HTTP-layer decentralisation.
