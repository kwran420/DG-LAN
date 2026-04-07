# Project Guidelines

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
- always use the build-release.ps1 script for building and releasing — do not run build tools manually
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
