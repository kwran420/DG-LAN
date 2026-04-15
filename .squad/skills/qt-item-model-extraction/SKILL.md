# Skill: Qt Item-Model Extraction

## When to Use
Use when a Qt Widgets screen has become a mega-widget because it owns both the visible controls and a large amount of `QStandardItemModel` bookkeeping.

## Pattern
1. Move the row/role bookkeeping into a plain helper that owns the `QStandardItemModel`.
2. Put shared column and role constants in one header so the widget, delegates, and helper stay aligned.
3. Keep the widget responsible for composition, signal wiring, and user actions; keep the helper responsible for row creation, state projection, and per-row metadata.
4. Preserve the existing model roles and visible columns during the extraction so behavior stays stable while the responsibilities split.
5. Re-run the repo validation entrypoint and report blocked desktop coverage honestly if the Qt toolchain is unavailable.

## DG-LAN Example
- `Browse/NetworkWidget` now delegates file-row creation, browse-generation pruning, transfer progress projection, upload/download speed display, and local-path lookup to `Browse/NetworkFileModel`.
- The widget still owns browse requests, context menus, queue move commands, and button enablement.
