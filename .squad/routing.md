# Work Routing

How to decide who handles what.

## Routing Table

| Work Type | Route To | Examples |
|-----------|----------|----------|
| Architecture, modernization, cross-cutting review | Ripley | Module boundaries, upgrade plans, reviewer gates, system-wide trade-offs |
| GUI, Qt widgets, UX, desktop modularisation | Dallas | MainWindow, NetworkWidget, dialogs, styling, UI cleanup |
| Core networking, protocols, services, Python bridge backends | Hicks | Peer discovery, remote control, file management, APIs, transfer logic |
| Testing, evaluation, regression safety rails | Vasquez | Test plans, harnesses, edge cases, validation strategy, quality gates |
| Documentation, migration notes, pruning, release-facing guidance | Bishop | README/build docs, deprecation plans, modernization recommendations |
| Code review | Ripley | Review PRs, check quality, suggest improvements |
| Testing | Vasquez | Write tests, find edge cases, verify fixes |
| Scope & priorities | Ripley | What to build next, trade-offs, decisions |
| Session logging | Scribe | Automatic — never needs routing |
| Work monitoring | Ralph | Backlog scans, issue pickup, follow-up chaining |

## Issue Routing

| Label | Action | Who |
|-------|--------|-----|
| `squad` | Triage: analyze issue, assign `squad:{member}` label | Ripley |
| `squad:{name}` | Pick up issue and complete the work | Named member |

### How Issue Assignment Works

1. When a GitHub issue gets the `squad` label, the **Lead** triages it — analyzing content, assigning the right `squad:{member}` label, and commenting with triage notes.
2. When a `squad:{member}` label is applied, that member picks up the issue in their next session.
3. Members can reassign by removing their label and adding another member's label.
4. The `squad` label is the "inbox" — untriaged issues waiting for Lead review.

## Rules

1. **Eager by default** — spawn all agents who could usefully start work, including anticipatory downstream work.
2. **Scribe always runs** after substantial work, always as `mode: "background"`. Never blocks.
3. **Quick facts → coordinator answers directly.** Don't spawn an agent for "what port does the server run on?"
4. **When two agents could handle it**, pick the one whose domain is the primary concern.
5. **"Team, ..." → fan-out.** Spawn all relevant agents in parallel as `mode: "background"`.
6. **Anticipate downstream work.** If a feature is being built, spawn the tester to write test cases from requirements simultaneously.
7. **Issue-labeled work** — when a `squad:{member}` label is applied to an issue, route to that member. The Lead handles all `squad` (base label) triage.
