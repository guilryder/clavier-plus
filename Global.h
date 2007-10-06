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
extern HWND e_hdlgModal;
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


// Display a message box. The message is read from string resources.
int messageBox(HWND hWnd, UINT idString, UINT uType = MB_ICONERROR, LPCTSTR pszArg = NULL);

// Centers a window relatively to its parent.
void centerParent(HWND hWnd);

// Reads the text of a dialog box control.
void getDlgItemText(HWND hDlg, UINT id, String& rs);

// Setups a label control for displaying an URL.
//
// Args:
//   hDlg: The handle of the dialog box containing the control to setup.
//   idControl: The ID of the label control in the dialog box.
//   pszLink: The URL to give to the control. It must be static buffer: the string will not
//     be copied to a buffer.
void initializeWebLink(HWND hDlg, UINT idControl, LPCTSTR pszLink);

// Wrapper for CreateThread()
void startThread(LPTHREAD_START_ROUTINE pfn, void* pParams);

// Writes a NULL-terminated string to a file.
void writeFile(HANDLE hf, LPCTSTR psz);

// Retrieves the executable name of the process associated to a window.
//
// Args:
//   hWnd: The window to get the executable of.
//   pszExecutableName: The buffer where to put the executable name, without path.
//     Should have a size of MAX_PATH.
//
// Returns:
//   True on success (pszPath contains a path), false on failure.
bool getWindowExecutable(HWND hWnd, LPTSTR pszExecutableName);

// Sleeps in idle priority.
//
// Args:
//   dwDurationMS: The time to sleep, in milliseconds.
void sleepBackground(DWORD dwDurationMS);

// Wrapper for SHGetFileInfo() that does not call the function if the file belongs
// to a slow device. If the call to SHGetFileInfo() fails and SHGFI_USEFILEATTRIBUTES was not
// specified in uFlags, the flag is added and SHGetFileInfo() is called again.
//
// Args:
//   pszPath: The path of the file or directory to call SHGetFileInfo() on.
//   dwFileAttributes: Given as argument to SHGetFileInfo().
//   shfi: Where to save the result of SHGetFileInfo().
//   uFlags: Given as argument to SHGetFileInfo(). If pszPath is on a slow device,
//     according pathIsSlow(), SHGFI_USEFILEATTRIBUTES is added to uFlags before it is given
//     to SHGetFileInfo().
//
// Returns:
//   True on success, false on failure.
bool getFileInfo(LPCTSTR pszPath, DWORD dwFileAttributes, SHFILEINFO& shfi, UINT uFlags);

void clipboardToEnvironment();

HWND findVisibleChildWindow(HWND hwndParent, LPCTSTR pszClass, bool bPrefix);
bool checkWindowClass(HWND hWnd, LPCTSTR pszClass, bool bPrefix);
HWND findWindowByName(LPCTSTR pszWindowSpec);

// Determines if a string matches a wildcards pattern, by ignoring case.
//
// Args:
//   pszPattern: The pattern to use for testing matching. Supports '*' and '?'.
//     This string must be given in lower case.
//   pszSubject: The string to test against the pattern. The case does not matter.
//
// Returns:
//   True if the subject matches the pattern, false otherwise.
bool matchWildcards(LPCTSTR pszPattern, LPCTSTR pszSubject);


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
