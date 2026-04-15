CONFIG(debug, debug|release) {
   FOLDER = debug
   DEFINES += DEBUG
} else {
   FOLDER = release

   # LTO (link-time optimisation)
   # gcc-ar is required so static archives keep LTO metadata.
   # Disabled by default on Linux because GCC ≥ 13 triggers an ICE when
   # linking mixed LTO objects + static libs.  Define ENABLE_LTO to opt in.
   unix {
      contains(DEFINES, ENABLE_LTO) {
         QMAKE_AR = gcc-ar cqs
         QMAKE_CXXFLAGS_RELEASE += -flto
         QMAKE_LFLAGS_RELEASE += -flto
      }
   }

   prof {
      QMAKE_CXXFLAGS += -pg -g
      QMAKE_LFLAGS += -pg
      QMAKE_LFLAGS_RELEASE -= -Wl,-s
   }
}

CONFIG += exceptions rtti
DEFINES *= QT_USE_QSTRINGBUILDER

DESTDIR = output/$$FOLDER
MOC_DIR = .tmp/$$FOLDER
OBJECTS_DIR = .tmp/$$FOLDER
RCC_DIR = .tmp/$$FOLDER
UI_DIR = .tmp/$$FOLDER

QMAKE_CXXFLAGS += -std=c++17
QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-parentheses
