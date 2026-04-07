---
description: "Test generation workflow. Analyzes code under test, follows existing patterns, and generates comprehensive tests covering happy paths, edge cases, and error scenarios."
agent: "agent"
argument-hint: "Describe what to test or reference the code..."
---
# Write Tests

Generate comprehensive tests for the specified code.

## Step 1: Analyze Code Under Test
- Read and understand the code to be tested
- Identify public interfaces, inputs, outputs, and side effects
- Note dependencies that need mocking
- Identify edge cases and error conditions

## Step 2: Research Existing Patterns
- Find existing tests in the project to match conventions:
  - Test framework and assertion library in use
  - File naming and location patterns
  - Setup/teardown patterns
  - Mocking approach and preferred libraries
- Follow these patterns exactly — consistency matters

## Step 3: Design Test Cases
For each function/method/component, plan tests for:

### Happy Path
- Normal expected usage with valid inputs
- Core business logic scenarios
- Standard state transitions

### Edge Cases
- Boundary values (min, max, zero, empty)
- Special characters in string inputs
- Large inputs or collections
- Concurrent access (if applicable)

### Error Scenarios
- Invalid inputs (wrong type, format, range)
- Missing required data (null, undefined, empty)
- External dependency failures (network, database, file system)
- Authorization/authentication failures (if applicable)

## Step 4: Write Tests
- Use the AAA pattern: Arrange → Act → Assert
- One assertion per concept — test one behavior per test case
- Descriptive names: `"should [behavior] when [condition]"`
- Use literal expected values — never compute expected values using implementation logic
- Mock only at external boundaries — use real implementations for internal logic
- Ensure each test is independent, creates its own data, and cleans up after itself

## Step 5: Verify
- Run all new tests — confirm they pass
- Run the full test suite — confirm no regressions
- Check that tests actually fail when the implementation is broken (test the test)
- No debug statements or commented-out test code
