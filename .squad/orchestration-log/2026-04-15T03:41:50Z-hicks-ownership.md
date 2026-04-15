# Orchestration Log — Hicks Ownership Refactor Slice

**Timestamp**: 2026-04-15T03:41:50Z  
**Agent**: Hicks (Backend)  
**Batch**: Peer Lifecycle Coverage Hardening  
**Status**: COMPLETED  

## Scope

Harden peer lifecycle management in DownloadManager by migrating long-lived download bookkeeping from raw IPeer* pointers to peer identity (Hash) storage.

## Decision Foundation

Move long-lived download bookkeeping to `Common::Hash` peer IDs first (`OccupiedPeers`, `LinkedPeers`) instead of full IPeer* ownership rewrite. This removes highest-risk stale-pointer maps/sets without changing protocol behavior.

## Deliverables

### 1. Occupancy Tracking Refactor
- **OccupiedPeers**: Migrated from raw `IPeer*` storage to `Common::Hash`-keyed tracking
- **LinkedPeers**: Updated peer ID bookkeeping; maintains same interface contract
- **DownloadManager**: Bookkeeping maps converted to peer ID keys instead of raw pointers
- Integration with Ripley's `peerBecomesUnavailable` signal for cleanup boundaries

### 2. Lifecycle Seam Preservation
- Existing `peerBecomesUnavailable` / `peerBecomesAvailable` boundaries remain unchanged
- Cleanup and reattachment logic uses signal-based notifications as primary path
- Transient raw `IPeer*` handles inside active transfers preserved (ChunkDownloader path)
- Future slice can wrap those live handles if full migration desired

### 3. Risk Mitigation
- Removes stale-pointer exposure from long-term bookkeeping (scheduler-visible state)
- Transient raw handles in ChunkDownloader remain defended by lazy `isAvailable()` checks
- No protocol-level behavior changes; internal safety improvement only

## Code Changes Summary

| Area | Change | Rationale |
|------|--------|-----------|
| `OccupiedPeers.{h,cpp}` | Hash-keyed storage + signal cleanup | Removes pointer-lifetime risk |
| `LinkedPeers.{h,cpp}` | Peer ID bookkeeping | Consistent with occupancy model |
| `DownloadManager.{h,cpp}` | Connect to cleanup signals; ID-based maps | Proactive stale-pointer removal |
| `ChunkDownloader` | No changes (transient handles) | Defense-in-depth; lazy checks sufficient |

## Validation

- **Occupancy maps**: Confirmed Hash-key migration complete; no dangling peer pointers
- **Signal integration**: peerBecomesUnavailable triggers cleanup correctly
- **Behavior preservation**: Download scheduling unchanged; cleanup faster
- **Code review**: Peer ID migration reviewed; no semantic changes to scheduler

## Risk Assessment

- **Risk Reduction**: HIGH — removes stale-pointer vectors from persistent scheduler state
- **Effort**: Complete
- **Timeline**: 2026-04-15
- **Remaining work**: ChunkDownloader can wrap transient handles in future slice (not blocking)

## Next Steps

- Full IPeer* → QSharedPointer migration blocked pending Ripley's signal infrastructure (now complete)
- ChunkDownloader transient handle wrapping deferred to future slice if desired
- Continue downstream cleanup signal propagation to other subsystems

---

**Completed by**: Hicks (Backend)  
**Implementation date**: 2026-04-15  
**Peer lifecycle seam**: peerBecomesUnavailable  
**Co-authored**: Copilot  
**Status**: ✅ READY FOR TEAM REVIEW
