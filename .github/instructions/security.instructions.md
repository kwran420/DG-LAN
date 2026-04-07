---
description: "Use when implementing authentication, authorization, data validation, API endpoints, database queries, handling sensitive data, encryption, or security reviews. Covers OWASP Top 10 awareness, input validation, access control, and secure defaults."
---
# Security Principles

## Input Validation
- Validate all external input at system entry points for expected format, type, and length
- Never trust client-side validation alone — always validate server-side
- Use allowlists over denylists when possible
- Reject invalid input early — fail fast

## Database Security
- Use parameterized queries (prepared statements) for **all** database access — no string concatenation for queries
- Apply least-privilege database accounts — only permissions required for the operation
- Validate and sanitize any dynamic column or table names against an allowlist if unavoidable

## Output Encoding
- Encode output appropriately for its rendering context (HTML, SQL, shell, URL, JSON)
- Return only information necessary for the caller in error responses
- Log detailed diagnostics server-side — never expose internal errors to users
- Never include stack traces, internal paths, or system details in user-facing responses

## Authentication & Authorization
- Apply authentication to all entry points that handle user data or trigger state changes
- Verify authorization for each resource access, not only at the entry point
- Use established authentication libraries/frameworks — never roll your own crypto
- Implement proper session management with secure token handling

## Secrets Management
- Store credentials and secrets through environment variables or dedicated secret managers
- Never hardcode secrets, API keys, or passwords in source code
- Never commit secrets to version control — use `.gitignore` and pre-commit hooks
- Rotate secrets regularly and support rotation without downtime

## Cryptography
- Use established cryptographic libraries provided by the language or framework
- Generate security-critical values (tokens, IDs, nonces) with cryptographically secure random generators
- Encrypt sensitive data at rest and in transit using standard protocols (TLS 1.2+)
- Never implement custom cryptographic algorithms

## Access Control
- Grant only the permissions required for the operation (files, database connections, API scopes)
- Apply least privilege to service accounts, file permissions, and network access
- Check authorization at every layer — not just the top-level handler
- Log access control failures for monitoring and alerting

## OWASP Top 10 Awareness
When implementing security-sensitive code, consider these common vulnerability categories:
1. **Broken Access Control** — Verify authorization for every action
2. **Cryptographic Failures** — Use strong, standard encryption; protect data at rest and in transit
3. **Injection** — Parameterized queries, input validation, output encoding
4. **Insecure Design** — Threat modeling, secure defaults, fail-safe mechanisms
5. **Security Misconfiguration** — Hardened defaults, no unnecessary features enabled
6. **Vulnerable Components** — Keep dependencies updated, monitor for CVEs
7. **Authentication Failures** — Strong credential handling, MFA where appropriate
8. **Data Integrity Failures** — Verify software updates and CI/CD pipelines
9. **Logging & Monitoring Failures** — Log security events, set up alerting
10. **Server-Side Request Forgery (SSRF)** — Validate and sanitize all URLs, restrict outbound requests

## Dependency Security
- Keep dependencies updated — monitor for known vulnerabilities
- Use lock files to pin dependency versions
- Review new dependencies before adding them — check maintenance status, known issues, and license compatibility
- Minimize dependency surface — only include what you actually need
