---
description: "Use when designing APIs, creating endpoints, defining request/response schemas, implementing REST services, or discussing API versioning, pagination, and error handling."
---
# API Design Principles

## RESTful Conventions

### HTTP Methods
| Method | Purpose | Idempotent |
|--------|---------|------------|
| `GET` | Retrieve resource(s) | Yes |
| `POST` | Create a new resource | No |
| `PUT` | Replace a resource entirely | Yes |
| `PATCH` | Update part of a resource | No* |
| `DELETE` | Remove a resource | Yes |

### Resource Naming
- Use nouns, not verbs: `/users` not `/getUsers`
- Use plural nouns for collections: `/users`, `/orders`
- Use kebab-case for multi-word resources: `/order-items`
- Nest for relationships: `/users/{id}/orders`
- Keep URLs shallow — maximum 2 levels of nesting

### Status Codes
| Range | Meaning | Common Codes |
|-------|---------|-------------|
| 2xx | Success | `200 OK`, `201 Created`, `204 No Content` |
| 3xx | Redirection | `301 Moved`, `304 Not Modified` |
| 4xx | Client error | `400 Bad Request`, `401 Unauthorized`, `403 Forbidden`, `404 Not Found`, `409 Conflict`, `422 Unprocessable Entity` |
| 5xx | Server error | `500 Internal Server Error`, `503 Service Unavailable` |

Use the most specific applicable status code — not `200` for everything.

## Error Responses

### Consistent Error Format
Use a single, consistent error response structure across all endpoints:

```json
{
  "error": {
    "code": "VALIDATION_FAILED",
    "message": "Human-readable description of what went wrong",
    "details": [
      {
        "field": "email",
        "message": "Must be a valid email address"
      }
    ]
  }
}
```

### Error Rules
- Always return a machine-readable error code alongside the message
- Include field-level details for validation errors
- Never expose internal error details, stack traces, or system paths to clients
- Log detailed error information server-side for debugging

## Input Validation
- Validate all inputs at the API boundary — type, format, length, range
- Return `400 Bad Request` with specific field-level error details for invalid input
- Use allowlists for enumerated values
- Set reasonable limits on string lengths, array sizes, and numeric ranges
- Reject unexpected fields in strict mode or ignore them silently — be consistent

## Pagination

### Offset-Based (simple)
```
GET /users?page=2&per_page=25
```

### Cursor-Based (preferred for large/changing datasets)
```
GET /users?cursor=abc123&limit=25
```

### Response Envelope
```json
{
  "data": [...],
  "pagination": {
    "total": 150,
    "page": 2,
    "per_page": 25,
    "next_cursor": "abc456"
  }
}
```

- Always include pagination metadata in list responses
- Set sensible defaults and maximums for page sizes
- Document the default and maximum page size

## Versioning
- Use URL path versioning as default: `/api/v1/users`
- Increment major version only for breaking changes
- Support at least one previous version during transition periods
- Document deprecation timelines clearly

## Request/Response Design
- Use consistent field naming (camelCase or snake_case — pick one and stick with it)
- Use ISO 8601 for dates and times: `2026-04-07T10:30:00Z`
- Return only the data the client needs — avoid over-fetching
- Use envelope pattern for list responses; direct object for single resource responses
- Include resource IDs in all response objects

## Authentication & Authorization
- Use standard auth mechanisms (OAuth 2.0, JWT, API keys)
- Pass tokens via `Authorization` header, not URL parameters
- Return `401 Unauthorized` for missing/invalid auth, `403 Forbidden` for insufficient permissions
- Rate-limit endpoints appropriately — return `429 Too Many Requests` with `Retry-After` header

## Documentation
- Document every endpoint: method, path, parameters, request body, response, error codes
- Include request/response examples for each endpoint
- Document authentication requirements
- Keep API docs in sync with implementation — update in the same commit
