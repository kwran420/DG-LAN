---
description: "Use when creating commits, writing commit messages, creating branches, preparing pull requests, merging code, or discussing version control workflow."
---
# Git Conventions

## Commit Messages

### Conventional Commits Format
```
type(scope): description

[optional body]

[optional footer]
```

### Types
| Type | Use For |
|------|---------|
| `feat` | New feature or capability |
| `fix` | Bug fix |
| `docs` | Documentation only changes |
| `refactor` | Code change that neither fixes a bug nor adds a feature |
| `test` | Adding or correcting tests |
| `chore` | Build process, tooling, or auxiliary changes |
| `perf` | Performance improvement |
| `ci` | CI/CD configuration changes |
| `build` | Build system or external dependency changes |

### Rules
- **Scope** is optional but encouraged — use the module or feature name
- **Description** starts lowercase, no period at the end, imperative mood ("add" not "added")
- **Body** explains *what* and *why*, not *how* — wrap at 72 characters
- **Breaking changes**: Add `!` after type/scope: `feat(api)!: remove deprecated endpoint`
- **Footer**: Reference issues with `Closes #123` or `Fixes #456`

### Examples
```
feat(auth): add JWT token refresh endpoint

Tokens now auto-refresh 5 minutes before expiration.
The refresh endpoint validates the existing token before issuing a new one.

Closes #142
```

```
fix(validation): handle empty email field in registration form
```

```
refactor(orders): extract discount calculation into separate module
```

## Atomic Commits
- Each commit represents one logical change
- The codebase should build and pass tests at every commit
- Don't mix refactoring with feature work in the same commit
- Don't mix formatting changes with logic changes

## Branches

### Naming Convention
```
type/short-description
```

### Examples
| Pattern | Example |
|---------|---------|
| Feature | `feature/add-user-auth` |
| Bug fix | `fix/login-validation-error` |
| Refactor | `refactor/extract-payment-module` |
| Docs | `docs/update-api-reference` |
| Chore | `chore/upgrade-dependencies` |

### Rules
- Use lowercase with hyphens (kebab-case)
- Keep descriptions short but descriptive (2–4 words)
- Delete branches after merging

## Pull Requests

### PR Description Template
Every PR description should cover:

1. **What** — What changed? (brief summary)
2. **Why** — Why was this change needed? (link to issue/requirement)
3. **How** — How was it implemented? (approach, key decisions)
4. **Testing** — How was it tested? (test types, manual verification)

### PR Hygiene
- Keep PRs focused — one feature or fix per PR
- Add reviewers explicitly
- Respond to review feedback promptly
- Squash commits if the history is noisy, preserve if each commit is meaningful
- Ensure CI passes before requesting review

## Pre-Commit Checklist
- [ ] All tests pass
- [ ] No debug statements (console.log, print, debugger)
- [ ] No commented-out code
- [ ] No TODO/FIXME without a linked issue
- [ ] No secrets or credentials in code
- [ ] Commit message follows conventional format
