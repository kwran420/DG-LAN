---
description: "End-to-end feature implementation workflow. Analyzes requirements, researches codebase, designs approach, implements with tests, and self-reviews."
agent: "agent"
argument-hint: "Describe the feature to implement..."
---
# Implement Feature

Implement the requested feature using a structured, end-to-end workflow.

## Guiding Principles
- **Simplicity First**: Implement the simplest solution that meets the requirements
- **Plan before building**: For 3+ step features, use the todo list to plan before touching code
- **Demand Elegance**: For non-trivial work, pause and ask "is there a more elegant way?" — skip for simple, obvious changes
- **Verify before done**: Never mark complete without proving it works — run tests, demonstrate correctness

## Step 1: Analyze Requirements
- Parse the feature request to identify scope, acceptance criteria, and constraints
- Determine complexity: simple (single file change), moderate (multiple files), or complex (cross-cutting)
- Identify any ambiguities and state your assumptions

## Step 2: Research Codebase
- Search for existing patterns, conventions, and related code
- Identify where the new code should live based on project structure
- Note any existing utilities, helpers, or abstractions to reuse
- Check for existing tests to understand testing patterns

## Step 3: Design Approach
- Propose the implementation approach with brief rationale
- For moderate/complex features: list the files to create or modify
- Consider at least one alternative approach and explain why you chose the primary one
- Identify risks or edge cases upfront

## Step 4: Implement
- Follow existing code patterns and project conventions
- Write clean, well-structured code following coding principles
- Keep changes focused — only what's needed for the feature
- Create small, logical units of work

## Step 5: Write Tests
- Write tests alongside or immediately after implementation
- Follow existing test patterns in the codebase
- Cover: happy path, edge cases, error scenarios
- Ensure tests are independent, deterministic, and fast

## Step 6: Self-Review
- Verify implementation matches the requirements and acceptance criteria
- Check for security concerns (input validation, auth, data exposure)
- Confirm no debug statements, commented-out code, or leftover artifacts
- Ensure all tests pass
- Summarize what was implemented and any decisions made
