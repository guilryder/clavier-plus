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
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


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
const size_t bufCode = 32;
const size_t bufWindowTitle = 256;

const int bufClipboardString = MAX_PATH * 10;


const LPCTSTR pszApp = _T("Clavier+");


extern HANDLE e_hHeap;
extern HINSTANCE e_hInst;
extern HWND e_hwndInvisible;  // Invisible background window
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


// Warning-free equivalents of macros GetWindowLongPtr, SetWindowLongPtr and SubclassWindow.
#pragma warning(disable: 4244)
inline LONG_PTR getWindowLongPtr(HWND hwnd, int index) {
	return GetWindowLongPtr(hwnd, index);
}

inline LONG_PTR setWindowLongPtr(HWND hwnd, int index, LONG_PTR new_long) {
	return SetWindowLongPtr(hwnd, index, new_long);
}

inline WNDPROC subclassWindow(HWND hwnd, WNDPROC new_window_proc) {
	return reinterpret_cast<WNDPROC>(setWindowLongPtr(hwnd, GWLP_WNDPROC,
		reinterpret_cast<LONG_PTR>(new_window_proc)));
}
#pragma warning(default: 4244)

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

HWND findVisibleChildWindow(HWND hwnd_parent, LPCTSTR pszClass, bool bPrefix);
bool checkWindowClass(HWND hWnd, LPCTSTR pszClass, bool bPrefix);
HWND findWindowByName(LPCTSTR pszWindowSpec);

// Determines if a string matches a wildcards pattern, by ignoring case.
//
// Args:
//   pattern: The pattern to use for testing matching. Supports '*' and '?'.
//     This string must be given in lower case.
//   subject: The string to test against the pattern. The case does not matter.
//   pattern_end: If not null, points to the successor of the last character of the pattern; if
//     equals pattern, the pattern will be considered empty.
//
// Returns:
//   True if the subject matches the pattern, false otherwise.
bool matchWildcards(LPCTSTR pattern, LPCTSTR subject, LPCTSTR pattern_end = NULL);


//------------------------------------------------------------------------
// SHBrowseForFolder wrapper:
// - use custom title
// - set initial directory
// - return directory path instead of LPITEMIDLIST
//------------------------------------------------------------------------

bool browseForFolder(HWND hwnd_parent, LPCTSTR pszTitle, LPTSTR pszDirectory);


//------------------------------------------------------------------------
// Shell API tools
//------------------------------------------------------------------------

bool getSpecialFolderPath(int index, LPTSTR pszPath);
bool getShellLinkTarget(LPCTSTR pszLinkFile, LPTSTR pszTargetPath);


//------------------------------------------------------------------------
// Application-specific helpers
//------------------------------------------------------------------------

// Returns the translation of a token in the current language.
LPCTSTR getToken(int tok);

// Returns the name of a language in this language.
LPCTSTR getLanguageName(int lang);

// Returns the index of a token from its name. The name of the token can be in any language.
//
// Returns:
//   A tok* enum value, or tokNotFound if the token does not match any token.
int findToken(LPCTSTR token);

// Increments a string pointer until the end of the string or a comma is encountered, then skips
// spaces. The comma is replaced by a null character, so that the initial pointer (before being
// incremented) will point to the string before the comma.
//
// Args:
//   rpc: A reference to the pointer to increment. The string initially pointed will be unescaped if
//     bUnescape is true.
//   bUnescape: If true, unescapes the string according to backslashes. The string is unescaped in
//     place, which is possible because the unescaping process can only reduce the string length.
void skipUntilComma(TCHAR*& rpc, bool bUnescape = false);


//------------------------------------------------------------------------
// Strings translation
//------------------------------------------------------------------------

#include "I18n.h"

// A string translated in all available languages.
class TranslatedString {
public:
	
	// Loads the translation of the string for the current language.
	void load(UINT id) {
		i18n::loadStringAuto(id, m_translations[i18n::getLanguage()]);
	}
	
	// Sets the translation of the string for the current language.
	void set(LPCTSTR psz) {
		lstrcpy(m_translations[i18n::getLanguage()], psz);
	}
	
	// Returns the translation of the string for a given language.
	LPCTSTR get(int lang) const {
		return m_translations[lang];
	}
	
	// Returns the translation of the string for the current language.
	LPCTSTR get() const {
		return get(i18n::getLanguage());
	}
	
private:
	
	// Translations of the string, indexed by language.
	TCHAR m_translations[i18n::langCount][bufString];
};


#include "MyString.h"


//------------------------------------------------------------------------
// Command line parsing and executing
//------------------------------------------------------------------------

void findFullPath(LPTSTR pszPath, LPTSTR pszFullPath);

void shellExecuteCmdLine(LPCTSTR pszCommand, LPCTSTR pszDirectory, int nShow);


struct THREAD_SHELLEXECUTE {
	THREAD_SHELLEXECUTE(const String& sCommand, const String& sDirectory, int nShow)
		: sCommand(sCommand), sDirectory(sDirectory), nShow(nShow) {}
	
	String sCommand;
	String sDirectory;
	int nShow;
};

DWORD WINAPI threadShellExecute(void* pParams);


//------------------------------------------------------------------------
// Settings loading and saving
//------------------------------------------------------------------------

enum {
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
