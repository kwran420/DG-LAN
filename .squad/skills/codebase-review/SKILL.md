# Skill: Codebase Architecture Review

## When to Use
When performing a full architecture review of a C++/Qt project with mixed subsystems.

## Pattern

1. **Parallelise exploration** — launch separate explore agents for each major subsystem (Core, GUI, Common, external bridges) simultaneously.
2. **Verify line counts** — always `wc -l` the god class candidates reported by exploration. Agent estimates can drift.
3. **Check for test infrastructure first** — knowing what's tested vs untested determines whether refactoring is safe.
4. **Trace raw pointer ownership** — in Qt/C++ codebases, search for `new` without corresponding `QSharedPointer`/`unique_ptr`. Peer lifetime across subsystem boundaries is the highest-risk pattern.
5. **Look for double-compilation** — when multiple `.pro` files include the same source (especially generated protobuf), flag it.
6. **Anchor recommendations to LOC** — "split this 912-line file" is actionable; "improve modularity" is not.

## Output Template
- Executive summary (1 paragraph)
- Highest-risk issues (ranked, with file paths and line counts)
- Modularisation roadmap (phased, with week estimates)
- Modernisation options (table: option / pros / cons / verdict)
- Pruning candidates (safe-to-delete vs archive vs verify-with-owner)
- Execution order (dependency-aware phases)
- Numbered team decisions

## Anti-Patterns to Avoid
- Don't recommend rewrites without acknowledging the working test suites that would be lost
- Don't count prototype/legacy code in active LOC metrics
- Don't recommend language migration without a phase gate (prototype → evaluate → decide)
