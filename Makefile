# OS dependend macros
IDIR = -Isrc/include
LIBS = -lcairo -lmuparser
DEFINES = -DBUILD_DLL
ADDITIONAL = -msse2 -std=c++11
ifeq ($(OS),Windows_NT)
IDIR += -Isrc/include/cairo -Isrc/include/muparser
LDIR = -Lsrc/libs/win32 -Lsrc/libs/win64
LIBS += -lcomdlg32 -lgdi32 -lpixman-1 -lpng -lz
DEFINES += -D_WIN32 -DWIN32_LEAN_AND_MEAN -DWIN32_EXTRA_LEAN
ADDITIONAL += -static
else
IDIR += -I/usr/include/cairo -I/usr/include/pango-1.0  -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include
LDIR =
LIBS += -lpango
DEFINES += -D__unix__
endif

# Constant macros
CC = g++
RC = windres
WARNINGS = -Winit-self -Wredundant-decls -Wundef -Wfloat-equal -Wunreachable-code -Wmissing-include-dirs -Wswitch-enum -pedantic -Wextra -Wall
OPTIMIZATION = -Os -O2
RDIR = -Isrc/res
CFLAGS = $(IDIR) $(WARNINGS) $(DEFINES) $(OPTIMIZATION) $(ADDITIONAL)
LFLAGS = -shared -Wl,--dll $(LDIR) $(LIBS)
RFLAGS = -J rc -O coff $(RDIR) $(DEFINES)

# Macro overwrite by build type
ifeq ($(BUILD),debug)
CFLAGS += -g -pg -DDEBUG
LFLAGS += -lgmon -pg
RFLAGS += -DDEBUG
else
CFLAGS += -s
LFLAGS += -s
endif

# Build binary
all: Dirs SSBRenderer
ifeq ($(OS),Windows_NT)
SSBRenderer: Renderer.o SSBParser.o aegisub.o avisynth.o user.o virtualdub.o cairo++.o module.o FileReader.o resources.res
	$(CC) -Wl,--output-def=bin/SSBRenderer.def -Wl,--out-implib=bin/SSBRenderer.a src/obj/Renderer.o src/obj/SSBParser.o src/obj/aegisub.o src/obj/avisynth.o src/obj/user.o src/obj/virtualdub.o src/obj/cairo++.o src/obj/FileReader.o src/obj/module.o src/obj/resources.res $(LFLAGS) -o bin/SSBRenderer.dll
else
SSBRenderer: Renderer.o SSBParser.o aegisub.o user.o cairo++.o FileReader.o
	$(CC) -Wl,--output-def=bin/SSBRenderer.def -Wl,--out-implib=bin/SSBRenderer.a src/obj/Renderer.o src/obj/SSBParser.o src/obj/aegisub.o src/obj/user.o src/obj/cairo++.o src/obj/FileReader.o $(LFLAGS) -o bin/SSBRenderer.so
endif

# Build single objects
Dirs:
	mkdir -p src/obj bin
Renderer.o:
	$(CC) $(CFLAGS) -c src/Renderer.cpp -o src/obj/Renderer.o
SSBParser.o:
	$(CC) $(CFLAGS) -c src/SSBParser.cpp -o src/obj/SSBParser.o
aegisub.o:
	$(CC) $(CFLAGS) -c src/aegisub.cpp -o src/obj/aegisub.o
avisynth.o:
	$(CC) $(CFLAGS) -c src/avisynth.cpp -o src/obj/avisynth.o
user.o:
	$(CC) $(CFLAGS) -c src/user.cpp -o src/obj/user.o
virtualdub.o:
	$(CC) $(CFLAGS) -c src/virtualdub.cpp -o src/obj/virtualdub.o
cairo++.o:
	$(CC) $(CFLAGS) -c src/cairo++.cpp -o src/obj/cairo++.o
FileReader.o:
	$(CC) $(CFLAGS) -c src/FileReader.cpp -o src/obj/FileReader.o
module.o:
	$(CC) $(CFLAGS) -c src/module.c -o src/obj/module.o
resources.res:
	$(RC) $(RFLAGS) -i src/resources.rc -o src/obj/resources.res

# Remove generated files
clean:
	rm -rf src/obj bin

# Install
install:
	cp bin/SSBRenderer.so /usr/local/include/SSBRenderer.so
	cp src/user.h /usr/local/include/ssb.h
uninstall:
	rm /usr/local/include/SSBRenderer.so
	rm /usr/local/include/ssb.h