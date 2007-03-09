// Clavier+
// Keyboard shortcuts manager
//
// Copyright (C) 2000-2007 Guillaume Ryder
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

#include "Resource.h"


#ifdef UNICODE
typedef WORD UTCHAR;
#else
typedef unsigned char UTCHAR;
#endif

class String;


const size_t bufString = 128;
const size_t bufHotKey = 128;
const size_t bufCode   =  32;

const int bufClipboardString = MAX_PATH * 10;


const LPCTSTR pszApp = _T("Clavier+");


extern HANDLE e_hHeap;
extern HINSTANCE e_hInst;
extern HWND e_hwndInvisible;    // Invisible background window
extern bool e_bIconVisible;


#ifdef UNICODE
#define strToW(buf, to, from) \
	const LPCWSTR to = from;
#else
#define strToW(buf, to, from) \
	WCHAR to[buf]; \
	MultiByteToWideChar(CP_ACP, 0, from, -1, to, nbArray(to));
#endif

#ifdef UNICODE
#define strToA(buf, to, from) \
	char to[buf]; \
	WideCharToMultiByte(CP_ACP, 0, from, -1, to, nbArray(to), NULL, NULL);
#else
#define strToA(buf, to, from) \
	const LPCSTR to = from;
#endif



int messageBox(HWND hWnd, UINT idString, UINT uType = MB_ICONERROR, LPCTSTR pszArg = NULL);

void centerParent(HWND hDlg);

void getDlgItemText(HWND hDlg, UINT id, String& rs);

void initializeWebLink(HWND hDlg, UINT idControl, LPCTSTR pszLink);

// Wrapper for CreateThread()
void startThread(LPTHREAD_START_ROUTINE pfn, void* pParams);

#define myGetProcAddress(hDLL, functionName) \
	((PFN_##functionName)GetProcAddress(hDLL, (#functionName ANSI_UNICODE("A", "W"))))

bool getWindowExecutable(HWND hWnd, LPTSTR pszPath);
void sleepBackground(DWORD dwDurationMS);

bool getFileInfo(LPCTSTR pszFile, DWORD dwAttributes, SHFILEINFO& shfi, UINT uFlags);

void clipboardToEnvironment();

HWND findVisibleChildWindow(HWND hwndParent, LPCTSTR pszClass, bool bPrefix);
bool checkWindowClass(HWND hWnd, LPCTSTR pszClass, bool bPrefix);


//------------------------------------------------------------------------
// SHBrowseForFolder wrapper:
// - use custom title
// - set initial directory
// - return directory path instead of LPITEMIDLIST
//------------------------------------------------------------------------

bool browseForFolder(HWND hwndParent, LPCTSTR pszTitle, LPTSTR pszDirectory);


//------------------------------------------------------------------------
// Shell API tools
//------------------------------------------------------------------------

bool getSpecialFolderPath(int index, LPTSTR pszPath);
bool getShellLinkTarget(LPCTSTR pszLinkFile, LPTSTR pszTargetPath);

void writeFile(HANDLE hf, LPCTSTR psz);

LPCTSTR getToken(int tok);
int findToken(LPCTSTR pszToken);


//------------------------------------------------------------------------
// Strings translation
//------------------------------------------------------------------------

#include "Lang.h"

class TranslatedString
{
public:
	
	void load(UINT id)
	{
		loadStringAuto(id, m_apsz[e_lang]);
	}
	
	void set(LPCTSTR psz)
	{
		lstrcpy(m_apsz[e_lang], psz);
	}
	
	LPCTSTR get(int lang) const { return m_apsz[lang]; }
	LPCTSTR get() const { return get(e_lang); }
	
	
private:
	
	TCHAR m_apsz[langCount][bufString];
};


#include "MyString.h"



//------------------------------------------------------------------------
// Command line parsing and executing
//------------------------------------------------------------------------

void findFullPath(LPTSTR pszPath, LPTSTR pszFullPath);

void shellExecuteCmdLine(LPCTSTR pszCommand, LPCTSTR pszDirectory, int nShow);


struct THREAD_SHELLEXECUTE
{
	THREAD_SHELLEXECUTE(const String& sCommand, const String& sDirectory, int nShow)
		: sCommand(sCommand), sDirectory(sDirectory), nShow(nShow) {}
	
	String sCommand;
	String sDirectory;
	int    nShow;
};

DWORD WINAPI threadShellExecute(void* pParams);


//------------------------------------------------------------------------
// Settings loading and saving
//------------------------------------------------------------------------

enum
{
	colContents,
	colShortcut,
	colCond,
	colDescription,
	colCount
};

const int nbColSize = colCount - 1;

extern int e_acxCol[colCount];

extern SIZE e_sizeMainDialog;
extern bool e_bMaximizeMainDialog;

extern TCHAR e_pszIniFile[MAX_PATH];

HKEY openAutoStartKey(LPTSTR pszPath);
