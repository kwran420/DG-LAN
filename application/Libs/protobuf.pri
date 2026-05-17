win32 {
   PROTOBUF_PREFIX = C:/msys64/mingw64
   INCLUDEPATH += $$PROTOBUF_PREFIX/include
   DEFINES += NOMINMAX

   PROTOBUF_PKGCONFIG = $$system(pkg-config --libs --static protobuf)
   isEmpty(PROTOBUF_PKGCONFIG) {
      error("pkg-config could not find protobuf. Install mingw-w64-x86_64-pkgconf and mingw-w64-x86_64-protobuf in MSYS2.")
   }

   PROTOBUF_STATIC_LIBS =
   PROTOBUF_SYSTEM_LIBS =
   for(lib, PROTOBUF_PKGCONFIG) {
      contains(lib, "^-l(advapi32|bcrypt|dbghelp)$$") {
         PROTOBUF_SYSTEM_LIBS += $$lib
      } else {
         PROTOBUF_STATIC_LIBS += $$lib
      }
   }

   LIBS += -L$$PROTOBUF_PREFIX/lib \
      -Wl,-Bstatic \
      $$PROTOBUF_STATIC_LIBS \
      -Wl,-Bdynamic \
      $$PROTOBUF_SYSTEM_LIBS
}

unix {
   LIBS += -lprotobuf
}
