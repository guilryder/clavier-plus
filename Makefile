# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Makefile to build clavier-plus with MingW-64 GCC
# This makefile avoids using the std lib (-nostdlib) so it is quite small.
# All is compiled in one line with g++ with lto (link-time-optimization)
# The generated file is around 200kB
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

CC=g++
WR=windres

LDFLAGS=-nostdlib -luuid -ladvapi32 -luser32 -lgdi32 -lcomctl32 \
	-lcomdlg32 -lshlwapi -lole32 -lmsi -lshell32 -lkernel32 -lpsapi \
	-Wl,-dynamicbase,-nxcompat,--no-seh \
	-Wl,--disable-reloc-section,--disable-runtime-pseudo-reloc \
	-Wl,--tsaware,-s -s -mwindows

# 32 bit Mode (uncomment if you need and comment the 64bit mode)
# We use PSAPI_VERSION=1 to link directly to psapi.dll
#CFLAGS=-m32 -march=i386 -mpreferred-stack-boundary=2 -DPSAPI_VERSION=1
#WRFLAGS=-Fpe-i386
#LDFLAGS+=-e_WinMainCRTStartup

# 64 bit Mode
#CFLAGS=-m64 -march=x86-64
#WRFLAGS=-Fpe-x86-64
#LDFLAGS+=-eWinMainCRTStartup

WARNINGS=-Wall -Wno-unknown-pragmas -Wno-multichar -Wno-strict-aliasing

# We need to use UNICODE and std=c++17
# Also there are a lot of flags necessary for the -nostdlib
# flag (/NODEFAULTLIBS equivalent)
CFLAGS+=-std=c++17 -Os -mno-stack-arg-probe -momit-leaf-frame-pointer \
	-fomit-frame-pointer -fno-stack-check -fno-stack-protector \
	-fno-threadsafe-statics -fno-use-cxa-get-exception-ptr \
	-fno-access-control -fno-enforce-eh-specs -fno-nonansi-builtins \
	-fnothrow-opt -fno-optional-diags -fno-use-cxa-atexit -fno-exceptions\
	-fno-dwarf2-cfi-asm -fno-asynchronous-unwind-tables \
	-fno-extern-tls-init -fno-rtti -Wnoexcept \
	-D_UNICODE -DUNICODE -U_DEBUG -UDEBUG -DNO_PROPSYS $(WARNINGS)

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
default: clavier.exe

clavier.exe: rez.o App.cpp App.h Dialogs.cpp Dialogs.h Global.cpp \
	Global.h I18n.cpp I18n.h Intrinsics.cpp Keystroke.cpp Keystroke.h \
	MyString.h Shortcut.cpp Shortcut.h StdAfx.cpp StdAfx.h
	$(CC) *.cpp rez.o -o clavier.exe $(CFLAGS) $(LDFLAGS) -flto

rez.o: Clavier.rc Resource.h Clavier.manifest App.ico Add.ico
	$(WR) $(WRFLAGS) Clavier.rc rez.o

clean:
	rm clavier.exe rez.o
