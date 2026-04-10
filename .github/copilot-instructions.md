# Project Guidelines

## Core Principles
- **Simplicity First**: Make every change as simple as possible — minimal code, minimal impact
- **No Laziness**: Find root causes, not temporary fixes — hold to senior developer standards
- **Minimal Impact**: Changes should only touch what's necessary — avoid introducing bugs

## Workflow Orchestration

### Plan Before Building
- Use the todo list to plan ANY non-trivial task (3+ steps or architectural decisions)
- If something goes sideways, STOP and re-plan immediately — don't keep pushing
- Plan verification steps, not just building
- Write detailed specs upfront to reduce ambiguity

### Subagent Strategy
- Use subagents (defined in `.github/agents/`) liberally to keep main context window clean
- Offload research, exploration, and parallel analysis to subagents
- For complex problems, throw more compute at it via subagents
- One task per subagent for focused execution

### Verification Before Done
- Never mark a task complete without proving it works
- Diff behavior between main and your changes when relevant
- Ask yourself: "Would a staff engineer approve this?"
- Run tests, check logs, demonstrate correctness

### Demand Elegance (Balanced)
- For non-trivial changes: pause and ask "is there a more elegant way?"
- If a fix feels hacky: "Knowing everything I know now, implement the elegant solution"
- Skip this for simple, obvious fixes — don't over-engineer
- Challenge your own work before presenting it

### Autonomous Bug Fixing
- When given a bug report: just fix it — don't ask for hand-holding
- Point at logs, errors, failing tests — then resolve them
- Zero context switching required from the user
- Go fix failing CI tests without being told how

## Task Management
1. **Plan First**: Use the todo list to plan work with checkable items
2. **Verify Plan**: Check in before starting implementation
3. **Track Progress**: Mark todo items complete as you go — one at a time
4. **Explain Changes**: High-level summary at each step
5. **Document Results**: Summarize outcomes when work is complete
6. **Capture Lessons**: Save lessons to memory (`/memories/repo/lessons.md`) after corrections

## Self-Improvement Loop
- After ANY correction from the user: save the lesson to memory (`/memories/repo/lessons.md`)
- Write rules for yourself that prevent the same mistake
- Ruthlessly iterate on these lessons until mistake rate drops
- Review repo memory at session start for relevant project lessons

## Code Style
- Use meaningful, descriptive names drawn from the problem domain
- Keep functions small and focused — under 50 lines, single responsibility
- Use early returns to reduce nesting; maximum 3 levels of nesting
- 0–2 parameters per function; group 3+ into objects/structs
- Extract magic numbers and strings into named constants
- Delete unused code — do not comment it out (retrieve from git history if needed)
- Remove all debug statements (console.log, print, debugger) before committing

## Architecture
- Separate concerns: domain logic, data access, and presentation in distinct layers
- Inject dependencies as parameters; depend on abstractions, not concrete implementations
- One primary responsibility per file; avoid files exceeding 500 lines
- Group related functionality together; keep high cohesion within modules and low coupling between them

## Build & Test
- Run the full test suite before committing — all tests must pass
- Write tests alongside implementation (TDD encouraged)
- Minimum 80% code coverage for production code
- Never skip or comment out failing tests — fix or delete them

## Conventions
- **Commits**: Use [Conventional Commits](https://www.conventionalcommits.org/) format: `type(scope): description`
  - Types: `feat`, `fix`, `chore`, `docs`, `refactor`, `test`, `perf`, `ci`, `build`
- **Branches**: `type/short-description` (e.g., `feature/add-auth`, `fix/login-bug`)
- **PRs**: Include what changed, why, how, and what testing was done
- **No leftover artifacts**: No TODO/FIXME without a linked issue, no commented-out code, no debug statements
