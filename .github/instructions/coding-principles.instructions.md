---
description: "Use when implementing features, writing new code, refactoring, or reviewing code quality. Covers function design, error handling, code organization, and dependency management."
---
# Coding Principles

## Function Design

### Parameters
- **0–2 parameters** per function is ideal
- For 3+ parameters, group into an object, struct, or dictionary
- Use descriptive parameter names that convey purpose

### Single Responsibility
- Each function does one thing well
- Keep functions under 50 lines
- Extract complex logic into separate, well-named functions
- Maintain a single level of abstraction per function

### Control Flow
- Use early returns to reduce nesting
- Maximum 3 levels of nesting — flatten with early returns or extracted functions
- Prefer guard clauses at function entry points
- Pure functions when possible (no side effects)

## Error Handling
- **Fail fast**: Detect and report errors as early as possible
- **Never swallow errors**: Always handle, log with context, or propagate explicitly
- Provide meaningful error messages with sufficient context for debugging
- Protect sensitive data — mask passwords, tokens, and PII in logs
- Use language-appropriate error handling mechanisms (exceptions, Result types, error codes)
- Include error context when re-throwing or wrapping errors

## Code Organization
- **One primary responsibility per file** — avoid god files exceeding 500 lines
- Group related functionality together within modules
- High cohesion within modules, low coupling between them
- Clear folder structure that reflects the architecture
- Separate concerns: domain logic, data access, and presentation in distinct layers

## Dependency Management
- Inject external dependencies as parameters (constructor injection for classes, function parameters for procedural/functional code)
- Depend on abstractions, not concrete implementations
- Minimize inter-module dependencies
- Facilitate testing through mockable dependency boundaries

## Naming
- Use meaningful, descriptive names drawn from the problem domain
- Use full words — abbreviations only when widely recognized in the domain
- Single-letter names only for loop counters or well-known conventions (i, j, x, y)
- Extract magic numbers and strings into named constants
- Follow the project's established naming conventions consistently

## Refactoring
- **Small steps**: One change at a time, keeping tests passing throughout
- **Verify behavior**: Run tests after every change
- Delete unused code — retrieve from git history if needed later
- Never comment out code as a way to "save" it

### Refactoring Triggers
- Code duplication (DRY principle)
- Functions exceeding 50 lines
- Complex conditional logic (deeply nested or long chains)
- Unclear naming or structure
- Files exceeding 500 lines

## Performance
- **Measure first**: Profile before optimizing — never optimize on gut feeling
- Focus on algorithmic complexity over micro-optimizations
- Choose data structures based on actual access patterns
- Handle resources properly (memory, connections, file handles)
- Optimize only after identifying actual bottlenecks through profiling

## Comments
- Comment the "what" and "why" — the code communicates the "how"
- Document public APIs and interfaces
- Note known limitations or edge cases
- Delete commented-out code (use git history)
- Write comments that remain accurate regardless of future changes — avoid references to dates, versions, or temporary state
- Update comments when changing the code they describe
