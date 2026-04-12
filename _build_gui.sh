#!/bin/bash
QMAKE=$(command -v qmake-qt5 2>/dev/null || command -v qmake)
cd /c/Dev/DG-LAN/application
$QMAKE GUI.pro -r -spec win32-g++ "CONFIG+=release"
mingw32-make -f Makefile-GUI -j4 2>&1 | tail -80