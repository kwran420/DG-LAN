# Security Policy

## Supported Versions

| Version | Supported |
|---------|-----------|
| Latest release | Yes |
| Older releases | No |

Only the latest release receives security fixes. Update to the latest version from [GitHub Releases](https://github.com/kwran420/DG-LAN/releases).

## Scope

DG-LAN is a **LAN file-sharing application** designed for trusted local networks (home, office, LAN parties). It is not designed for use on the public internet.

Security-relevant areas include:
- **Core ↔ GUI protocol** (TCP, protobuf): localhost by default, but can connect remotely
- **Peer-to-peer protocol** (UDP/TCP): multicast, broadcast, and direct connections on the LAN
- **File indexing and transfers**: reading/writing shared files
- **Auto-update system**: downloads installers from GitHub Releases
- **`dglan://` URL scheme**: processes URLs from the browser

## Reporting a Vulnerability

If you discover a security vulnerability:

1. **Do not** open a public GitHub issue.
2. **Email** the maintainer directly or use [GitHub Security Advisories](https://github.com/kwran420/DG-LAN/security/advisories/new) to report privately.
3. Include:
   - Description of the vulnerability
   - Steps to reproduce
   - Potential impact
   - Suggested fix (if any)

You will receive a response acknowledging the report. Fixes will be released as soon as practical.

## Security Considerations

- **Network trust**: DG-LAN trusts all peers on the configured network interface. Only run it on networks you trust.
- **Remote Core**: If connecting the GUI to a remote Core, use a VPN or trusted network. The protocol does not use TLS.
- **URL scheme**: The `dglan://` handler validates parameters before processing. Malformed URLs are rejected.
- **Auto-update**: Updates are downloaded from GitHub over HTTPS. The installer is executed locally.
