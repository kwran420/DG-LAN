# Skill: Qt Dead Widget Pruning

## When to Use
Use when a Qt Widgets codebase has legacy surfaces that may be dead, but you need a safe first pruning slice.

## Pattern
1. Search for both class references and constructor calls to prove the widget is not instantiated anywhere reachable.
2. Check the active `.pro` file to see whether the sources are still compiled.
3. If a widget is uninstantiated **and** excluded from the project file, delete it outright.
4. If a widget is uninstantiated but still compiled, first remove it from the `.pro` file to shrink the live build surface; only delete the source later if product intent stays dead.
5. Re-run the repo validation entrypoint and report any blocked desktop layer honestly.

## DG-LAN Example
- `ActivityWidget` and `HashingProgressWidget`: deleted after zero instantiation references and no `GUI.pro` entries.
- `UploadsWidget` / `UploadsModel`: removed from `GUI.pro` but left in-tree because uploads were previously deferred rather than fully cancelled.
