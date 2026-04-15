# Decision: Peer Death Notification Infrastructure (Ripley, 2026-04-15)

## Context
The first code-changing batch of the modernization/modularization/high-risk-reduction effort.

## Decision
Added `peerBecomesUnavailable` signal to `IPeerManager` as the symmetric pair to the existing `peerBecomesAvailable`. This is now the canonical way for any subsystem to react to a peer dying.

## Rationale
- The IPeer.h contract said "A peer is never deleted, it's safe to keep a pointer on it" — this was misleading. Peers become dead (isAlive() == false) regularly. The comment is now corrected.
- Without a death signal, DownloadManager relied on lazy `isAvailable()` checks, leaving stale entries in `OccupiedPeers` sets until the next scheduling round.
- This signal is the required foundation for D7 (QSharedPointer migration).

## Reviewer Gate
**D7 (IPeer* → QSharedPointer migration) may now proceed.** Hicks should:
1. Use `peerBecomesUnavailable` as the trigger for clearing QWeakPointer references
2. Start with ChunkDownloader (highest exposure surface)
3. Get Lead review before changing the IPeerManager public interface (getPeers(), getPeer() return types)

## Impact
- PeerManager, DownloadManager, OccupiedPeers modified
- No behavioral change for working downloads — only adds proactive cleanup path
- ARCHITECTURE.md updated with signal documentation
