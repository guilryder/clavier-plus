# Compiles Clavier+ with MinGW64.
# Do not use for official builds.
#
# Limitations:
# * Does not compile or run the tests.
# * Slow: no incremental compilation, no precompiled headers.
# * Generates binaries ~20% larger than MSVC.

CC=g++
WR=windres
OUTDIR=output\mingw

LDFLAGS=\
	-nostdlib \
	-ladvapi32 \
	-lcomctl32 \
	-lcomdlg32 \
	-lgdi32 \
	-lkernel32 \
	-lmsi \
	-lole32 \
	-lpropsys \
	-lpsapi \
	-lshell32 \
	-lshlwapi \
	-luser32 \
	-luuid \
	-Wl,-dynamicbase,-nxcompat,--no-seh \
	-Wl,--disable-reloc-section,--disable-runtime-pseudo-reloc \
	-Wl,--tsaware,-s \
	-s -mwindows

# 64 bit mode
CFLAGS=-m64 -march=x86-64
WRFLAGS=-Fpe-x86-64
LDFLAGS+=-eWinMainCRTStartup
CLDFLAGS=-flto  # link-time-optimization

WARNINGS=-Wall -Wno-unknown-pragmas -Wno-multichar -Wno-strict-aliasing

CFLAGS+=\
	-std=c++17 -Os \
	-fno-access-control \
	-fno-asynchronous-unwind-tables \
	-fno-dwarf2-cfi-asm \
	-fno-enforce-eh-specs \
	-fno-exceptions \
	-fno-extern-tls-init \
	-fno-nonansi-builtins \
	-fno-optional-diags \
	-fno-rtti \
	-fno-stack-check \
	-fno-stack-protector \
	-fno-threadsafe-statics \
	-fno-use-cxa-atexit \
	-fno-use-cxa-get-exception-ptr \
	-fnothrow-opt \
	-fomit-frame-pointer \
	-mno-stack-arg-probe \
	-momit-leaf-frame-pointer \
	-Wnoexcept \
	-D_UNICODE -DUNICODE -DNDEBUG -U_DEBUG -UDEBUG \
	$(WARNINGS)

.PHONY: pre-build

default: $(OUTDIR)\Clavier.exe

pre-build:
	-@if not exist $(OUTDIR) mkdir $(OUTDIR)

$(OUTDIR)\Clavier.exe: *.cpp *.h $(OUTDIR)\Resource.o
	$(CC) $(filter-out %.h,$^) -o "$@" $(CFLAGS) $(LDFLAGS) $(CLDFLAGS)

$(OUTDIR)\Resource.o: Clavier.rc Resource.h *.bmp *.cur *.ico *.manifest pre-build
	$(WR) $(WRFLAGS) "$<" -o "$@" -D INCLUDE_RAW_MANIFEST
