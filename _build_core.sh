#!/bin/bash
cd '/c/Dev/DG-LAN/application'
QMAKE=$(command -v qmake-qt5 2>/dev/null || command -v qmake)
$QMAKE Core.pro -r -spec win32-g++ 'CONFIG+=release'
mingw32-make -f Makefile-Core -j$(nproc) 2>&1
