---
description: "Code review workflow. Checks code against coding principles, security, testing, and performance standards. Produces structured feedback with severity levels."
agent: "agent"
argument-hint: "Specify the code or files to review..."
tools: [read, search]
---
# Review Code

Perform a structured code review of the specified code.

## Review Dimensions

### 1. Correctness
- Does the code do what it's supposed to do?
- Are edge cases handled?
- Are there off-by-one errors, race conditions, or logic flaws?

### 2. Code Quality
- Follows coding principles: small functions, meaningful names, single responsibility
- Appropriate error handling — no swallowed errors
- No code duplication — DRY without over-abstracting
- Clean control flow — early returns, minimal nesting

### 3. Security
- Input validation at system boundaries
- Parameterized database queries (no string concatenation)
- No hardcoded secrets or credentials
- Proper authentication and authorization checks
- Output encoding appropriate for context
- No sensitive data in logs or error responses

### 4. Testing
- Adequate test coverage for new/changed code
- Tests follow AAA pattern with descriptive names
- Edge cases and error paths tested
- Tests are independent and deterministic
- No skipped or commented-out tests

### 5. Performance
- No obvious performance issues (N+1 queries, unnecessary loops, missing indexes)
- Appropriate data structures for access patterns
- Resources properly managed (connections, file handles, memory)

### 6. Maintainability
- Code is self-documenting with clear naming
- No commented-out code or debug statements
- No TODO/FIXME without a linked issue
- Files within reasonable size limits

## Output Format

For each finding, report:

```
### [SEVERITY] Title
**File**: path/to/file
**Line(s)**: relevant line range
**Issue**: What's wrong
**Suggestion**: How to fix it
```

### Severity Levels
- **CRITICAL**: Security vulnerability, data loss risk, or broken functionality — must fix before merging
- **WARNING**: Code quality issue, missing tests, or potential bug — should fix before merging
- **SUGGESTION**: Style improvement, optimization opportunity, or minor enhancement — nice to have

## Summary
End with a brief overall assessment:
- Total findings by severity
- Overall code quality impression
- Recommendation: approve, approve with suggestions, or request changes
