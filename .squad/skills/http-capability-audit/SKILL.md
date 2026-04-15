# HTTP Capability Audit

## Purpose

Use this pattern when product language claims load balancing, decentralisation, streaming, or failover across multiple HTTP-related paths.

## Steps

1. Separate the surfaces first:
   - built-in/native HTTP
   - adapter/bridge HTTP
   - native custom-protocol handoff (for example `dglan://`)
2. For each surface, trace four things:
   - what docs claim
   - what code actually routes/serves
   - what tests currently cover
   - what runtime evidence exists in the current environment
3. Do not treat “distributed native download” as proof of “distributed HTTP serving.”
4. Check owner eligibility explicitly:
   - local-only
   - master-only
   - any peer with the asset
   - any peer with HTTP enabled
5. Check fallback semantics explicitly:
   - direct serve
   - redirect/forward
   - failover
   - loop prevention
6. Write acceptance criteria around truthfulness before performance language:
   - who can serve
   - how the server chooses
   - what happens when the chosen peer is stale/dead
   - whether rehosted client copies are eligible
7. Flag documentation overclaim whenever section titles or summaries promise stronger decentralisation than the routing code proves.

## DG-LAN Example

- Built-in C++ HTTP currently serves locally and redirects only to peers where `isMaster()` and `getHttpPort() > 0`.
- Python bridge HTTP currently serves only what the connected Core can resolve locally; it does not redirect to peers.
- `dglan://` handoff queues a native DG-LAN download, after which chunk ownership gossip can add multiple peers per download.
- Therefore: native download decentralisation is real evidence; decentralised HTTP load balancing still needs explicit proof.
