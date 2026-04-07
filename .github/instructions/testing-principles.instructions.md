---
description: "Use when writing tests, designing test strategies, reviewing test quality, setting up test infrastructure, or discussing TDD. Covers test-driven development, test design patterns, coverage standards, and test organization."
---
# Testing Principles

## Test-Driven Development (TDD)

### Red-Green-Refactor Cycle
1. **RED**: Write a failing test first — ensure it fails for the right reason
2. **GREEN**: Write minimal code to make the test pass — just enough, no more
3. **REFACTOR**: Improve code structure — eliminate duplication, improve naming, keep all tests passing
4. **VERIFY**: Run the full test suite — check for regressions

## Test Design

### AAA Pattern (Arrange-Act-Assert)
Structure every test in three clear phases:
```
// Arrange: Set up test data and conditions
// Act: Execute the code under test
// Assert: Verify expected outcome
```

### One Assertion Per Concept
- Test one behavior per test case
- Multiple assertions are fine if testing a single concept
- Split unrelated assertions into separate tests

### Descriptive Test Names
Format: `"should [expected behavior] when [condition]"`

Examples:
- `"should return error when email is invalid"`
- `"should calculate discount when user is premium"`
- `"should throw exception when file not found"`

## Test Types

### Unit Tests
- Test individual components in isolation
- Fast execution (< 100ms each)
- No external dependencies — mock external services
- Majority of your test suite

### Integration Tests
- Test interactions between components
- May include database, file system, or APIs
- Slower than unit tests (< 1s each)
- Verify contracts between modules

### End-to-End (E2E) Tests
- Test complete workflows from user perspective
- Simulate real user interactions
- Slowest test type, fewest in number
- Highest confidence level

## Quality Standards

### Coverage
- **Minimum 80% code coverage** for production code
- Prioritize critical paths and business logic over hitting a percentage
- Use coverage as a guide, not a goal — meaningful assertions matter more

### Test Characteristics (FIRST)
- **Fast**: Unit < 100ms, integration < 1s, full suite < 10 minutes
- **Independent**: No dependencies between tests, create own test data, clean up own state
- **Repeatable**: Same input always produces same output — deterministic
- **Self-checking**: Clear pass/fail without manual verification
- **Timely**: Written close to the code they test

## What to Test

### Focus on Behavior
- Test observable behavior, not implementation details
- Test through public interfaces — avoid testing private methods directly
- Test return values, outputs, exceptions, and side effects

### Always Test
- **Happy path**: Normal, expected usage
- **Boundary conditions**: Min/max values, empty collections
- **Error cases**: Invalid input, null values, missing data
- **Edge cases**: Special characters, extreme values

### Literal Expected Values
- Use hardcoded literal values in assertions — never compute expected values using the same logic as the implementation
- If expected value equals mock return value unchanged, the test verifies nothing

## Mocking

### When to Mock
- External dependencies: APIs, databases, file systems
- Slow operations: network calls, heavy computations
- Unpredictable behavior: random values, current time
- Unavailable services: third-party APIs

### Mocking Principles
- Mock at boundaries, not internally
- Keep mocks simple and focused
- Use real implementations for internal utilities and business logic
- Over-mocking reduces test value by verifying wiring instead of behavior

## Test Organization

### File Structure
- Mirror production code structure
- Follow the project's test file naming convention
- Group related tests together
- Separate test types: `unit/`, `integration/`, `e2e/` in separate directories

### Test Hygiene
- **Fix or delete failing tests** — never skip or comment them out
- Remove commented-out tests entirely
- No debug code left in tests
- No flaky tests — make them deterministic using fixed seeds, time mocking, and proper cleanup

## Before Committing
- All tests pass
- No tests skipped or commented out
- No debug code in tests
- Coverage meets standards
- Tests run within performance thresholds
