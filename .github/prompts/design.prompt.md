---
description: "Architecture and design planning workflow. Analyzes current codebase, identifies constraints, and produces a structured design proposal with alternatives and tradeoffs."
agent: "agent"
argument-hint: "Describe what needs to be designed..."
tools: [read, search, web]
---
# Design

Create a structured design proposal for the requested feature or system change.

## Step 1: Analyze Current State
- Examine the existing codebase architecture: layers, modules, data flow, dependencies
- Identify relevant existing patterns and conventions
- Note any technical debt or constraints that affect the design
- Understand the current test coverage and quality posture

## Step 2: Identify Constraints
- **Technical**: Library compatibility, infrastructure limitations, performance requirements
- **Business**: Timeline, scope, backward compatibility needs
- **Team**: Existing conventions, skill set, maintenance burden

## Step 3: Propose Architecture
Present the recommended design:

### Overview
- High-level description of the approach
- Key components and their responsibilities
- Data flow between components

### Detailed Design
- Files to create or modify
- Interface contracts between components
- Data models or schema changes
- External dependencies (if any)

### Alternatives Considered
For each alternative:
- Brief description of the approach
- Pros and cons compared to the recommended design
- Why it was not selected

## Step 4: Risk Assessment
- Identify potential risks and failure modes
- Propose mitigation strategies for each risk
- Note any assumptions that could invalidate the design

## Step 5: Implementation Strategy
- Recommend vertical slice (feature-driven) or horizontal slice (foundation-driven) approach
- Break down into implementation phases if complex
- Define verification criteria for each phase:
  - **L1**: Functional operation — feature works end-to-end
  - **L2**: Test verification — new tests added and passing
  - **L3**: Build success — code builds without errors

## Output
Present the design as a structured document that can be referenced during implementation.
