---
description: "Safe refactoring workflow. Ensures tests exist before changing code, makes small incremental changes, and verifies behavior is preserved after each step."
agent: "agent"
argument-hint: "Describe what to refactor and why..."
---
# Refactor

Refactor the specified code safely while preserving all existing behavior.

## Step 1: Identify Targets
- Analyze the code to identify specific refactoring targets and the reason for each:
  - Code duplication (DRY)
  - Long functions (> 50 lines)
  - Deep nesting (> 3 levels)
  - Large files (> 500 lines)
  - Poor naming or unclear structure
  - Tight coupling or missing abstractions
  - Mixed responsibilities in a single function/class/file

## Step 2: Verify Test Coverage
Before changing any code:
- Check if tests exist for the code being refactored
- If tests are missing or insufficient, **write characterization tests first** — tests that capture the current behavior, even if imperfect
- Run the existing test suite to establish the green baseline
- Note the current test count and coverage for the affected code

## Step 3: Plan Changes
- Break the refactoring into small, independent steps
- Each step should be a single, well-defined transformation:
  - Rename a variable or function
  - Extract a function or method
  - Move code to a new file
  - Simplify a conditional
  - Introduce a parameter object
  - Replace a magic number with a named constant
- Order steps to minimize risk — simplest/safest changes first

## Step 4: Execute Incrementally
For each planned step:
1. Make **one change only**
2. Run the full test suite — all tests must pass
3. If tests fail, revert the change and reassess
4. Move to the next step only when green

## Step 5: Final Verification
- Run the full test suite — confirm all tests pass
- Compare behavior before and after — no functional changes
- Review the refactored code against the original goals:
  - Is it more readable?
  - Is it better structured?
  - Is duplication reduced?
- Confirm no debug statements, commented-out code, or leftover artifacts
- Summarize what was changed and why
