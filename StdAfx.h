// Clavier+
// Keyboard shortcuts manager
//
// Copyright (C) 2000-2006 Guillaume Ryder
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


#pragma once

#pragma warning (disable: 4710 4711)

#define _WIN32_IE      0x0400
#define _WIN32_WINNT   0x0400
#define _WIN32_WINDOWS 0x0500
#define WINVER         0x0500
#define STRICT 1

#define __STDC_WANT_SECURE_LIB__ 0
#define _CRT_DISABLE_PERFCRIT_LOCKS

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
#define NOWH
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

#include <tchar.h>


// Memory leaks detection
#ifdef _DEBUG
#include <stdlib.h>
#include <crtdbg.h>
#define DEBUG_NEW      new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else
#define DEBUG_NEW      new
#endif

#define new DEBUG_NEW


#ifdef UNICODE
#define ANSI_UNICODE(ansi, unicode)   unicode
typedef WORD UCHAR;
#else
#define ANSI_UNICODE(ansi, unicode)   ansi
typedef unsigned char UCHAR;
#endif


#define ToBool(f)        ((f) != 0)
#define nbArray(a)       (sizeof(a)/sizeof(a[0]))
#define MYUNUSED(param)

#define VERIFP(f,value)  { if (!(f))  return (value); }
#define VERIF(f)         VERIFP(f,false)
#define VERIFV(f)        { if (!(f))  return; }

#define min(a,b)   (((a) < (b)) ? (a) : (b))
#define max(a,b)   (((a) > (b)) ? (a) : (b))
