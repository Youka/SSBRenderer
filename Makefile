# Check OS
ifneq ($(OS),Windows_NT)
ifneq ($(shell uname -s),Linux)
$(error Unsupported OS!)
endif
endif

# configs
ifneq ($(OS),Windows_NT)
ifneq ($(wildcard config.mak),)
include config.mak
endif
endif

# OS dependend macros
IDIR = -Isrc/include
LIBS = -lmuparser
DEFINES = -DBUILD_DLL
ADDITIONAL = -msse2 -std=c++11
ifeq ($(OS),Windows_NT)
IDIR += -Isrc/include/cairo -Isrc/include/muparser
LDIR = -Lsrc/libs/win32 -Lsrc/libs/win64
LIBS += -lcairo -lpixman-1 -lpng -lz -lcomdlg32 -lgdi32
DEFINES += -D_WIN32 -DWIN32_LEAN_AND_MEAN -DWIN32_EXTRA_LEAN
ADDITIONAL += -static
else
IDIR += -I/usr/include/cairo -I/usr/include/pango-1.0 `pkg-config --cflags glib-2.0`
LDIR =
LIBS += -lcairo -lpango-1.0 -lpangocairo-1.0 -lpthread
DEFINES += -D__unix__
ifeq ($(with_fPIC),yes)
ADDITIONAL += -fPIC
endif
endif

# Overridable macros
CXX := g++
AR := ar
RANLIB := ranlib

# Constant macros
RC = windres
WARNINGS = -Winit-self -Wredundant-decls -Wundef -Wfloat-equal -Wunreachable-code -Wmissing-include-dirs -Wswitch-enum -pedantic -Wextra -Wall
OPTIMIZATION = -Os -O2
RDIR = -Isrc/res
CFLAGS = $(IDIR) $(WARNINGS) $(DEFINES) $(OPTIMIZATION) $(ADDITIONAL)
LFLAGS = -shared $(LDIR) $(LIBS)
RFLAGS = -J rc -O coff $(RDIR) $(DEFINES)

# Macro overwrite by build type
ifeq ($(BUILD),debug)
CFLAGS += -g -pg -DDEBUG
LFLAGS += -pg
ifeq ($(OS),Windows_NT)
LFLAGS += -lgmon
endif
RFLAGS += -DDEBUG
else
CFLAGS += -s
LFLAGS += -s
endif


all: SSBRenderer $(SHAREDLIB) $(STATICLIB)

# Build binaries
ifeq ($(OS),Windows_NT)
SSBRenderer: Dirs Renderer.o SSBParser.o aegisub.o avisynth.o user.o virtualdub.o vapoursynth.o cairo++.o module.o FileReader.o resources.res
	$(CXX) -Wl,--dll -Wl,--output-def=bin/SSBRenderer.def -Wl,--out-implib=bin/SSBRenderer.a src/obj/Renderer.o src/obj/SSBParser.o src/obj/aegisub.o src/obj/avisynth.o src/obj/user.o src/obj/vapoursynth.o src/obj/virtualdub.o src/obj/cairo++.o src/obj/FileReader.o src/obj/module.o src/obj/resources.res $(LFLAGS) -o bin/SSBRenderer.dll
else
OBJFILES = src/obj/Renderer.o src/obj/SSBParser.o src/obj/aegisub.o src/obj/user.o src/obj/vapoursynth.o src/obj/cairo++.o src/obj/FileReader.o
OBJS = Renderer.o SSBParser.o aegisub.o user.o vapoursynth.o cairo++.o FileReader.o

SSBRenderer:

$(SHAREDLIB): Dirs $(OBJS)
	$(CXX) -Wl,-soname,$@.$(VERSION) $(OBJFILES) $(LFLAGS) -o bin/$@

$(STATICLIB): Dirs $(OBJS)
	$(AR) rc bin/$@ $(OBJFILES)
endif


# Build single objects
Dirs:
	mkdir -p src/obj bin
Renderer.o:
	$(CXX) $(CFLAGS) -c src/Renderer.cpp -o src/obj/Renderer.o
SSBParser.o:
	$(CXX) $(CFLAGS) -c src/SSBParser.cpp -o src/obj/SSBParser.o
aegisub.o:
	$(CXX) $(CFLAGS) -c src/aegisub.cpp -o src/obj/aegisub.o
avisynth.o:
	$(CXX) $(CFLAGS) -c src/avisynth.cpp -o src/obj/avisynth.o
user.o:
	$(CXX) $(CFLAGS) -c src/user.cpp -o src/obj/user.o
vapoursynth.o:
	$(CXX) $(CFLAGS) -c src/vapoursynth.cpp -o src/obj/vapoursynth.o
virtualdub.o:
	$(CXX) $(CFLAGS) -c src/virtualdub.cpp -o src/obj/virtualdub.o
cairo++.o:
	$(CXX) $(CFLAGS) -c src/cairo++.cpp -o src/obj/cairo++.o
FileReader.o:
	$(CXX) $(CFLAGS) -c src/FileReader.cpp -o src/obj/FileReader.o
module.o:
	$(CXX) $(CFLAGS) -c src/module.c -o src/obj/module.o
resources.res:
	$(RC) $(RFLAGS) -i src/resources.rc -o src/obj/resources.res


# Remove generated files
clean:
	rm -rf src/obj bin

distclean: clean
	rm -rf config.mak SSBRenderer.pc


# Install
ifneq ($(OS),Windows_NT)
install:
	install -d $(DESTDIR)$(libdir)/pkgconfig
	install -d $(DESTDIR)$(includedir)
	install -m644 src/user.h $(DESTDIR)$(includedir)/ssb.h
	install -m644 SSBRenderer.pc $(DESTDIR)$(pkgconfigdir)/SSBRenderer.pc
ifneq ($(SHAREDLIB),)
	install -m644 bin/$(SHAREDLIB) $(DESTDIR)$(libdir)/$(SHAREDLIB).$(VERSION)
	ln -s $(SHAREDLIB).$(VERSION) $(DESTDIR)$(libdir)/$(SHAREDLIB)
endif
ifneq ($(STATICLIB),)
	install -m644 bin/$(STATICLIB) $(DESTDIR)$(libdir)/$(STATICLIB)
endif

uninstall:
	rm -f $(DESTDIR)$(libdir)/$(SHAREDLIB).$(VERSION) $(DESTDIR)$(libdir)/$(STATICLIB) $(DESTDIR)$(libdir)/$(SHAREDLIB) $(DESTDIR)$(pkgconfigdir)/SSBRenderer.pc $(DESTDIR)$(includedir)/ssb.h
endif
