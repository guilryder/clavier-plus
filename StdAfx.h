// Clavier+
// Keyboard shortcuts manager
//
// Copyright (C) 2000-2008 Guillaume Ryder
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#pragma once

// Ignore inline expansion warnings.
#pragma warning (disable: 4710 4711)

#define STRICT  1

// Require compatibility with Windows 7 or later.
#define _WIN32_WINNT    _WIN32_WINNT_WIN7
#include <sdkddkver.h>

// Microsoft standard library configuration.
#define _HAS_STATIC_RTTI  0
#ifndef TESTING
#define __STDC_WANT_SECURE_LIB__  0
#define _CRT_DISABLE_PERFCRIT_LOCKS
#define _STRALIGN_USE_SECURE_CRT  0
#endif
#ifndef _DEBUG
#define _CRT_SECURE_INVALID_PARAMETER  __noop
#endif

// Do not include some Windows APIs we don't need.
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS
#define NOSYSCOMMANDS
#define NORASTEROPS
#define OEMRESOURCE
#define NOATOM
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOOPENFILE
#define NOSERVICE
#define NOSOUND
#define NOCOMM
#define NOKANJI
#define NOPROFILER
#define NOMCX


#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <commdlg.h>

#include <cstring>
#include <cwchar>
#include <tchar.h>

#define STRSAFE_NO_DEPRECATE
#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>

// Deprecate most functions that strsafe.h replaces.
// Allow wsprintf() and friends to avoid linking issues with StringCchPrint().

#undef strcpy
#define strcpy strcpy_instead_use_StringCbCopyA_or_StringCchCopyA;

#undef wcscpy
#define wcscpy wcscpy_instead_use_StringCbCopyW_or_StringCchCopyW;

#undef strcat
#define strcat strcat_instead_use_StringCbCatA_or_StringCchCatA;

#undef wcscat
#define wcscat wcscat_instead_use_StringCbCatW_or_StringCchCatW;

#undef strcpyA
#define strcpyA strcpyA_instead_use_StringCbCopyA_or_StringCchCopyA;

#undef strcpyW
#define strcpyW strcpyW_instead_use_StringCbCopyW_or_StringCchCopyW;

#undef lstrcpy
#define lstrcpy lstrcpy_instead_use_StringCbCopy_or_StringCchCopy;

#undef lstrcpyA
#define lstrcpyA lstrcpyA_instead_use_StringCbCopyA_or_StringCchCopyA;

#undef lstrcpyW
#define lstrcpyW lstrcpyW_instead_use_StringCbCopyW_or_StringCchCopyW;

#undef StrCpy
#define StrCpy StrCpy_instead_use_StringCbCopy_or_StringCchCopy;

#undef StrCpyA
#define StrCpyA StrCpyA_instead_use_StringCbCopyA_or_StringCchCopyA;

#undef StrCpyW
#define StrCpyW StrCpyW_instead_use_StringCbCopyW_or_StringCchCopyW;

#undef _tcscpy
#define _tcscpy _tcscpy_instead_use_StringCbCopy_or_StringCchCopy;

#undef _ftcscpy
#define _ftcscpy _ftcscpy_instead_use_StringCbCopy_or_StringCchCopy;

#undef lstrcat
#define lstrcat lstrcat_instead_use_StringCbCat_or_StringCchCat;

#undef lstrcatA
#define lstrcatA lstrcatA_instead_use_StringCbCatA_or_StringCchCatA;

#undef lstrcatW
#define lstrcatW lstrcatW_instead_use_StringCbCatW_or_StringCchCatW;

#undef StrCat
#define StrCat StrCat_instead_use_StringCbCat_or_StringCchCat;

#undef StrCatA
#define StrCatA StrCatA_instead_use_StringCbCatA_or_StringCchCatA;

#undef StrCatW
#define StrCatW StrCatW_instead_use_StringCbCatW_or_StringCchCatW;

#undef StrNCat
#define StrNCat StrNCat_instead_use_StringCbCatN_or_StringCchCatN;

#undef StrNCatA
#define StrNCatA StrNCatA_instead_use_StringCbCatNA_or_StringCchCatNA;

#undef StrNCatW
#define StrNCatW StrNCatW_instead_use_StringCbCatNW_or_StringCchCatNW;

#undef StrCatN
#define StrCatN StrCatN_instead_use_StringCbCatN_or_StringCchCatN;

#undef StrCatNA
#define StrCatNA StrCatNA_instead_use_StringCbCatNA_or_StringCchCatNA;

#undef StrCatNW
#define StrCatNW StrCatNW_instead_use_StringCbCatNW_or_StringCchCatNW;

#undef _tcscat
#define _tcscat _tcscat_instead_use_StringCbCat_or_StringCchCat;

#undef _ftcscat
#define _ftcscat _ftcscat_instead_use_StringCbCat_or_StringCchCat;

#ifdef __GNUC__
#define __noop  ((void)0)
#endif

// Test-friendly assert().
#ifdef _DEBUG
#include <stdexcept>

#define assert(expression) \
	(void)(!!(expression) || reportFailedAssert("assert: " #expression));

inline bool reportFailedAssert(const char* message) {
	throw std::runtime_error(message);
}

#else
#define assert(expression)  __noop
#endif

// Memory leaks detection
#ifdef _DEBUG
#include <stdlib.h>
#include <crtdbg.h>
#define DEBUG_NEW  new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else
#define DEBUG_NEW  new
#endif

#define new DEBUG_NEW

#define memcpy_s(dest, dest_size, src, size)  memcpy(dest, src, size)


#ifndef UNICODE
#error Non-Unicode is no longer supported
#endif


#ifdef __GNUC__
#define SUPPRESS_WARNING(code)
#else
#define SUPPRESS_WARNING(code)  __pragma(warning(suppress: code))
#endif

#define toBool(value)  ((value) != 0)
#define UNUSED(name)  SUPPRESS_WARNING(4100) name

template <class T, int size>
inline constexpr int arrayLength(const T(&)[size]) {
  return size;
}


#define VERIFP(f, value)  { if (!(f)) { return (value); } }
#define VERIF(f)  VERIFP(f, false)
#define VERIFV(f)  { if (!(f))  return; }
