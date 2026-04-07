---
description: "Problem diagnosis workflow. Collects evidence, generates hypotheses, evaluates each against evidence, and proposes solutions with tradeoff analysis. Use for bugs, errors, unexpected behavior, or performance issues."
agent: "agent"
argument-hint: "Describe the problem or error..."
---
# Diagnose

Investigate and diagnose the reported problem using a structured analytical approach.

## Step 1: Collect Evidence
Gather all available information:
- **Error messages**: Exact text, stack traces, error codes
- **Reproduction**: Steps to reproduce, frequency, environment specifics
- **Code**: Relevant source files, recent changes (git log/diff)
- **Logs**: Application logs, system logs, monitoring data
- **Context**: When did it start? What changed recently? Who is affected?

## Step 2: Generate Hypotheses
Based on the evidence, generate **3–5 possible root causes**, ranked by likelihood:

| # | Hypothesis | Likelihood | Supporting Evidence | Contradicting Evidence |
|---|-----------|-----------|-------------------|----------------------|
| 1 | ... | High/Medium/Low | ... | ... |
| 2 | ... | ... | ... | ... |

Consider:
- Recent code changes (most common cause)
- Configuration or environment changes
- External dependency issues
- Data quality or migration problems
- Concurrency or timing issues

## Step 3: Evaluate Hypotheses
For each hypothesis:
1. What specific evidence supports it?
2. What evidence contradicts it?
3. What additional test would confirm or rule it out?

Run the most efficient tests to narrow down:
- Add targeted logging or debugging
- Write a failing test that reproduces the bug
- Check specific code paths or data states
- Compare working vs broken environments

## Step 4: Identify Root Cause
- State the confirmed root cause with confidence level (high/medium/low)
- Explain the chain of causation: trigger → mechanism → symptom
- If multiple causes contribute, explain how they interact

## Step 5: Propose Solutions
Present **2–3 solutions** with tradeoff analysis:

| Solution | Pros | Cons | Effort | Risk |
|----------|------|------|--------|------|
| A: ... | ... | ... | Low/Med/High | Low/Med/High |
| B: ... | ... | ... | ... | ... |

Include:
- **Immediate fix**: Quickest path to resolve the symptom
- **Proper fix**: Addresses root cause fully
- **Preventive measure**: How to prevent recurrence (test, monitoring, validation)

## Step 6: Recommend
- State the recommended solution and why
- Provide concrete implementation steps
- Include a regression test that would catch this issue in the future
