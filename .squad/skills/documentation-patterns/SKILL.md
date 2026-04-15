# Documentation Patterns for Developer Velocity

**Context:** DG-LAN project review identified documentation gaps that block contributor onboarding and increase development risk.

## The Problem

Incomplete documentation leads to:
- Developers don't know how to run tests → regressions slip through
- Contributors don't understand code style → PR review friction
- Operators don't know how to deploy → production failures
- New contributors waste time guessing at architecture decisions

## The Pattern: Layered Documentation

Organize project docs into complementary layers, each answering a specific question:

### Layer 1: **README.md** (What & Why)
- **Audience:** End users, prospective contributors
- **Content:** Value proposition, quick start, feature list, basic architecture diagram
- **Length:** 1–2 pages (keep shallow)
- **Example:** DG-LAN README covers: overview, how it works, features, quick start, architecture diagram, built-in HTTP server

### Layer 2: **BUILD.md** (How to Build & Deploy)
- **Audience:** Contributors, release managers, operators
- **Content:** Prerequisites, build steps (per platform), running app, firewall config, URL scheme registration
- **Structure:** Primary platform first, experimental platforms second
- **Example:** DG-LAN BUILD.md covers: Windows primary (qmake + MSYS2), Linux/macOS experimental, ZeroTier setup, firewall rules

### Layer 3: **TESTING.md** (How to Verify)
- **Audience:** Contributors, CI/CD engineers
- **Content:** Running each test suite, adding tests, minimum coverage requirements, CI gates
- **Length:** 2–4 pages
- **Missing from DG-LAN:** Should cover C++ tests, Python tests, integration scenarios
- **Example structure:**
  ```markdown
  ## Setup
  ## Running Tests
  ### C++ Tests (qmake-based)
  - TestsCommon
  - TestsFileManager
  - TestsPeerManager
  ### Python Tests (pytest)
  - test_streamer.py (59 tests)
  ## Adding New Tests
  ## CI Requirements
  ```

### Layer 4: **CODE-STYLE.md** (How to Write Code)
- **Audience:** Contributors
- **Content:** Naming conventions, comment style, error handling, logging levels, Qt-specific patterns
- **Length:** 3–5 pages with examples
- **Missing from DG-LAN:** Should cover naming (camelCase vs snake_case), header guards, signal/slot lifecycle
- **Example structure:**
  ```markdown
  ## Naming Conventions
  - Classes: CamelCase (e.g., FileManager)
  - Methods: camelCase (e.g., getEntries)
  - Private members: m_name (Qt convention)
  ## Header Guards
  - Use #pragma once (simpler, modern)
  ## Error Handling
  - Return codes vs exceptions (document decision)
  ## Logging
  - L_DEBUG, L_INFO, L_WARN, L_ERROR (define levels)
  ```

### Layer 5: **PROJECT-CONTEXT.md** (How It Works Internally)
- **Audience:** Core contributors, AI assistants, future maintainers
- **Content:** Project structure, version system, architecture notes, CI/CD flow, design decisions
- **Length:** 5–10 pages
- **Example:** DG-LAN PROJECT-CONTEXT.md has excellent coverage of build system, version scheme, auto-update flow

### Layer 6: **ARCHITECTURE.md** (Deep Dives)
- **Audience:** Core team, system designers
- **Content:** Module dependencies, concurrency model, protocol specs, state machines, design rationale
- **Length:** 10+ pages
- **Missing from DG-LAN:** Should document peer discovery state machine, selective rehosting logic, thread safety model
- **Example structure:**
  ```markdown
  ## Module Dependencies
  (diagram showing Core modules)
  ## Concurrency Model
  - Thread pool, signal/slots, mutex rules
  ## Peer Discovery
  - Multicast → Broadcast → Scan → Gossip
  - State machine, timeout handling
  ## Protocol Versioning
  - gui_protocol.proto versioning strategy
  - Backward compatibility rules
  ```

### Layer 7: **OPERATIONS.md** (How to Run in Production)
- **Audience:** DevOps, sys admins, operators
- **Content:** Windows Service setup, config locations, monitoring, troubleshooting, performance tuning
- **Length:** 3–5 pages
- **Missing from DG-LAN:** Should cover service installation, log file locations, peer discovery troubleshooting
- **Example structure:**
  ```markdown
  ## Running as Windows Service
  ## Configuration
  - Registry keys, config file locations
  ## Monitoring
  - Health check endpoints, log analysis
  ## Troubleshooting
  - Peer discovery failures, slow indexing, network issues
  - Log file locations and rotation
  ```

### Layer 8: **API Reference** (Programmatic Details)
- **Audience:** Integration developers, maintainers of API bridges
- **Content:** HTTP endpoints, message formats, error codes, examples
- **Example:** DG-LAN's dglan-api/HTTP-STREAMING.md is excellent—comprehensive API spec with JavaScript examples

## Implementation Checklist for DG-LAN

- [ ] **README.md** ✓ (exists, strong)
- [ ] **BUILD.md** ✓ (exists, good)
- [ ] **TESTING.md** ❌ (missing—high priority)
- [ ] **CODE-STYLE.md** ❌ (missing—high priority)
- [ ] **PROJECT-CONTEXT.md** ✓ (exists, needs update for protocol/concurrency sections)
- [ ] **ARCHITECTURE.md** ❌ (missing—medium priority)
- [ ] **OPERATIONS.md** ❌ (missing—medium priority)
- [ ] **API Reference** ✓ (dglan-api docs are excellent)

## How to Use This Pattern

1. **When starting a new project:** Create this 8-layer structure from day one.
2. **When onboarding contributors:** Direct them to each layer in sequence.
3. **When adding features:** Update the relevant layer (usually PROJECT-CONTEXT.md or ARCHITECTURE.md).
4. **When something breaks in production:** Add troubleshooting to OPERATIONS.md.
5. **When code style issues arise in PR review:** Add the pattern to CODE-STYLE.md, then reference it.

## Pitfalls to Avoid

1. **Monolithic README:** Don't put everything in README. Link to other docs.
2. **Outdated docs:** When code changes, update docs immediately (enforce in PR CI).
3. **Duplicated info:** Each layer should have one authority. Link to it, don't repeat.
4. **Missing rationale:** Always explain *why* a design decision was made, not just *what* it is.
5. **Examples without context:** Every code example needs 1–2 sentences of setup context.

## Tools & Automation

- **GitHub wiki:** For operational runbooks (OPERATIONS.md, troubleshooting)
- **Markdown formatting:** Keep it simple, support rendering in GitHub + local editors
- **Cross-links:** Use relative paths and markdown links (`[TESTING.md](TESTING.md)`)
- **CI validation:** Add `markdownlint` to CI to catch formatting issues
- **Version sync:** When bumping version, auto-update VERSION in PROJECT-CONTEXT.md

## Measurable Success Criteria

- New contributors can build and test the project in <2 hours (without asking questions)
- 80%+ of PR review comments are about logic, not code style (after CODE-STYLE.md exists)
- Operators can deploy to production without calling the dev team
- Contributors confidently add new modules because they understand the architecture
- 100% of API changes are documented before merge

---

**Skill verified in:** DG-LAN (C++17 + Qt5 + Python project, ~500 files, 10+ subsystems)  
**Tested by:** Bishop (Docs/Modernization)  
**Date:** April 15, 2026
