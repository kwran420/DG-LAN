---
description: "Generate a structured pull request description from the current changes. Analyzes the diff and produces a PR description covering what changed, why, how, and testing done."
agent: "agent"
argument-hint: "Optionally describe the context or link to an issue..."
tools: [read, search]
---
# Create PR Description

Generate a structured pull request description for the current changes.

## Step 1: Analyze Changes
- Review all changed files (staged and unstaged)
- Categorize changes: new features, bug fixes, refactoring, documentation, tests, config
- Identify the primary purpose of the PR

## Step 2: Generate Description

Use this structure:

```markdown
## What
[One paragraph summary of what this PR does]

## Why
[Why was this change needed? Link to issue/requirement if applicable]

## How
[Technical approach — key decisions, patterns used, notable implementation details]

### Changes
- [Bulleted list of significant changes by file or module]

## Testing
- [What tests were added or modified]
- [Manual testing performed]
- [Edge cases verified]

## Checklist
- [ ] Tests pass
- [ ] No debug statements or commented-out code
- [ ] Follows project conventions
- [ ] Documentation updated (if applicable)
```

## Step 3: Determine PR Metadata
Suggest:
- **Title**: Following conventional commit format — `type(scope): description`
- **Labels**: Based on change type (feature, bug, refactor, docs, etc.)
- **Reviewers**: Based on files changed and code ownership (if CODEOWNERS exists)

## Notes
- Keep the description concise but complete — a reviewer should understand the PR without reading every line of code
- Highlight any breaking changes, migration steps, or deployment considerations prominently
- If the PR is large, suggest how it could be split for easier review
