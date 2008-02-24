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


#include "StdAfx.h"
#include "Global.h"

#include <intshcut.h>
#include <tlhelp32.h>
#include <psapi.h>

const int kWindowClassBufferLength = 200;

HANDLE e_hHeap;
HINSTANCE e_hInst;
HWND e_hwndInvisible;
HWND e_hdlgModal;
bool e_bIconVisible = true;

#define myGetProcAddress(hDLL, functionName) \
	((PFN_##functionName)GetProcAddress(hDLL, (#functionName ANSI_UNICODE("A", "W"))))


static bool pathIsSlow(LPCTSTR path);

static BOOL CALLBACK prcEnumFindWindowByName(HWND hwnd, LPARAM lParam);


int messageBox(HWND hwnd, UINT idString, UINT uType, LPCTSTR pszArg) {
	TCHAR pszFormat[256], pszText[1024];
	i18n::loadStringAuto(idString, pszFormat);
	wsprintf(pszText, pszFormat, pszArg);
	return MessageBox(hwnd, pszText, pszApp, uType);
}


void centerParent(HWND hwnd) {
	RECT rcParent, rcChild;
	
	const HWND hwnd_parent = GetParent(hwnd);
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	if (hwnd_parent) {
		const HMONITOR hMonitor = MonitorFromWindow(hwnd_parent, MONITOR_DEFAULTTONULL);
		VERIFV(hMonitor);
		GetMonitorInfo(hMonitor, &mi);
		GetWindowRect(hwnd_parent, &rcParent);
		IntersectRect(&rcParent, &rcParent, &mi.rcWork);
	} else {
		const HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		VERIFV(hMonitor);
		GetMonitorInfo(hMonitor, &mi);
		rcParent = mi.rcWork;
	}
	
	GetWindowRect(hwnd, &rcChild);
	SetWindowPos(hwnd, NULL,
		(rcParent.left + rcParent.right + rcChild.left - rcChild.right) / 2,
		(rcParent.top + rcParent.bottom + rcChild.top - rcChild.bottom) / 2,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}


void getDlgItemText(HWND hdlg, UINT id, String& rs) {
	const HWND hwnd = GetDlgItem(hdlg, id);
	const int buf = GetWindowTextLength(hwnd) + 1;
	GetWindowText(hwnd, rs.getBuffer(buf), buf);
}


static WNDPROC s_prcLabel;
static LRESULT CALLBACK prcWebLink(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void initializeWebLink(HWND hdlg, UINT control_id, LPCTSTR link) {
	const HWND hwnd = GetDlgItem(hdlg, control_id);
	setWindowLongPtr(hwnd, GWL_USERDATA, reinterpret_cast<LONG_PTR>(link));
	s_prcLabel = subclassWindow(hwnd, prcWebLink);
}

// Window procedure for dialog box controls displaying an URL.
// The link itself is stored in GWL_USERDATA.
LRESULT CALLBACK prcWebLink(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		
		case WM_SETCURSOR:
			SetCursor(LoadCursor(NULL, IDC_HAND));
			return TRUE;
		
		case WM_NCHITTEST:
			return HTCLIENT;
		
		case WM_LBUTTONDOWN:
			ShellExecute(hwnd, NULL, reinterpret_cast<LPCTSTR>(getWindowLongPtr(hwnd, GWL_USERDATA)),
				NULL, NULL, SW_SHOWDEFAULT);
			return 0;
	}
	
	return CallWindowProc(s_prcLabel, hwnd, uMsg, wParam, lParam);
}


// Wrapper for CreateThread().
void startThread(LPTHREAD_START_ROUTINE pfn, void* params) {
	DWORD idThread;
	CloseHandle(CreateThread(NULL, 0, pfn, params, 0, &idThread));
}


void writeFile(HANDLE hf, LPCTSTR strbuf) {
	DWORD len;
	WriteFile(hf, strbuf, lstrlen(strbuf) * sizeof(TCHAR), &len, NULL);
}


bool getWindowProcessName(HWND hwnd, LPTSTR process_name) {
	DWORD process_id;
	GetWindowThreadProcessId(hwnd, &process_id);
	
	const HANDLE process_handle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process_id);
	VERIF(process_handle);
	const bool ok = (GetProcessImageFileName(process_handle, process_name, MAX_PATH) > 0);
	CloseHandle(process_handle);
	
	if (ok) {
		PathStripPath(process_name);
		CharLower(process_name);
	}
	
	return ok;
}


void sleepBackground(DWORD dwDurationMS) {
	const HANDLE hProcess = GetCurrentProcess();
	const DWORD dwOldPriority = GetPriorityClass(hProcess);
	SetPriorityClass(hProcess, IDLE_PRIORITY_CLASS);
	Sleep(dwDurationMS);
	SetPriorityClass(hProcess, dwOldPriority);
}


// Indicates if a file is located in a slow drive (network, removable, etc.).
bool pathIsSlow(LPCTSTR path) {
	const int drive_index = PathGetDriveNumber(path);
	if (drive_index >= 0) {
		// The path has a drive
		TCHAR root[4];
		PathBuildRoot(root, drive_index);
		switch (GetDriveType(root)) {
			case DRIVE_UNKNOWN:
			case DRIVE_REMOVABLE:
			case DRIVE_REMOTE:
			case DRIVE_CDROM:
				return true;
		}
	} else if (path[0] == _T('\\') && path[1] == _T('\\')) {
		// Network path
		return true;
	}
	return false;
}


bool getFileInfo(LPCTSTR path, DWORD file_attributes, SHFILEINFO& shfi, UINT flags) {
	TCHAR pszPathTemp[MAX_PATH];
	
	if (pathIsSlow(path)) {
		// If UNC server or share, remove any trailing backslash
		// If normal file, strip the path: extension-based icon
		
		flags |= SHGFI_USEFILEATTRIBUTES;
		lstrcpy(pszPathTemp, path);
		PathRemoveBackslash(pszPathTemp);
		const LPCTSTR pszPathName = PathFindFileName(path);
		if (PathIsUNCServer(pszPathTemp) || PathIsUNCServerShare(pszPathTemp)) {
			path = pszPathTemp;
		} else if (pszPathName) {
			lstrcpy(pszPathTemp, pszPathName);
			path = pszPathTemp;
		}
	}
	
	for (;;) {
		const bool bOK = toBool(SHGetFileInfo(path, file_attributes,
				&shfi, sizeof(shfi), flags));
		if (bOK || (flags & SHGFI_USEFILEATTRIBUTES)) {
			return bOK;
		}
		flags |= SHGFI_USEFILEATTRIBUTES;
	}
}


void clipboardToEnvironment() {
	bool ok = false;
	
	const UINT clipboard_format = ANSI_UNICODE(CF_TEXT, CF_UNICODETEXT);
	
	if (IsClipboardFormatAvailable(clipboard_format) && OpenClipboard(NULL)) {
		const HGLOBAL clipboard_mem = static_cast<HGLOBAL>(GetClipboardData(clipboard_format));
		if (clipboard_mem) {
			const LPTSTR clipboard_text = static_cast<LPTSTR>(GlobalLock(clipboard_mem));
			if (clipboard_text) {
				if (lstrlen(clipboard_text) < bufClipboardString) {
					ok = true;
					SetEnvironmentVariable(clipboard_env_variable, clipboard_text);
				}
				GlobalUnlock(clipboard_mem);
			}
		}
		CloseClipboard();
	}
	
	if (!ok) {
		SetEnvironmentVariable(clipboard_env_variable, _T(""));
	}
}


HWND findVisibleChildWindow(HWND hwnd_parent, LPCTSTR wnd_class, bool allow_same_prefix) {
	HWND hwndChild = NULL;
	while (toBool(hwndChild = FindWindowEx(hwnd_parent, hwndChild, NULL, NULL))) {
		if ((GetWindowStyle(hwndChild) & WS_VISIBLE) &&
				checkWindowClass(hwndChild, wnd_class, allow_same_prefix)) {
			return hwndChild;
		}
	}
	return NULL;
}

bool checkWindowClass(HWND hwnd, LPCTSTR wnd_class, bool allow_same_prefix) {
	TCHAR actual_wnd_class[kWindowClassBufferLength];
	VERIF(GetClassName(hwnd, actual_wnd_class, arrayLength(actual_wnd_class)));
	if (allow_same_prefix) {
		actual_wnd_class[lstrlen(wnd_class)] = _T('\0');
	}
	return !lstrcmp(wnd_class, actual_wnd_class);
}

struct FIND_WINDOW_BY_NAME {
	LPCTSTR title_regexp;
	HWND hwnd_found;
};

HWND findWindowByName(LPCTSTR title_regexp) {
	const LPTSTR pszWindowSpecLowercase = new TCHAR[lstrlen(title_regexp) + 1];
	lstrcpy(pszWindowSpecLowercase, title_regexp);
	CharLower(pszWindowSpecLowercase);
	
	FIND_WINDOW_BY_NAME fwbn;
	fwbn.title_regexp = pszWindowSpecLowercase;
	fwbn.hwnd_found = NULL;
	EnumWindows(prcEnumFindWindowByName, (LPARAM)&fwbn);
	delete [] pszWindowSpecLowercase;
	return fwbn.hwnd_found;
}

BOOL CALLBACK prcEnumFindWindowByName(HWND hwnd, LPARAM lParam) {
	FIND_WINDOW_BY_NAME &fwbn = *(FIND_WINDOW_BY_NAME*)lParam;
	TCHAR title[1024];
	if (GetWindowText(hwnd, title, arrayLength(title))) {
		if (matchWildcards(fwbn.title_regexp, title)) {
			fwbn.hwnd_found = hwnd;
			return FALSE;
		}
	}
	return TRUE;
}


bool matchWildcards(LPCTSTR pattern, LPCTSTR subject, LPCTSTR pattern_end) {
	for (;;) {
		const TCHAR pattern_chr = (pattern == pattern_end) ? _T('\0') : *pattern++;
		switch (pattern_chr) {
			
			case _T('\0'):
				return !*subject;
			
			case _T('*'):
				if (!*pattern || pattern == pattern_end) {
					return true;  // Optimization
				}
				do {
					if (matchWildcards(pattern, subject, pattern_end)) {
						return true;
					}
				} while (*subject++);
				return false;
			
			case _T('?'):
				VERIF(*subject++);
				break;
			
			default:
				VERIF(*subject++ == pattern_chr);
				break;
		}
	}
}


//------------------------------------------------------------------------
// SHBrowseForFolder wrapper:
// - use given title
// - set initial directory
// - return directory path instead of LPITEMIDLIST
//------------------------------------------------------------------------

static int CALLBACK prcBrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

bool browseForFolder(HWND hwnd_parent, LPCTSTR title, LPTSTR directory) {
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(bi));
	bi.hwndOwner = hwnd_parent;
	bi.lpszTitle = title;
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = prcBrowseForFolderCallback;
	bi.lParam = (LPARAM)directory;
	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if (!pidl || !SHGetPathFromIDList(pidl, directory)) {
		return false;
	}
	CoTaskMemFree(pidl);
	return true;
}

int CALLBACK prcBrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM, LPARAM lpData) {
	if (uMsg == BFFM_INITIALIZED) {
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}
	return 0;
}


//------------------------------------------------------------------------
// Shell API tools
//------------------------------------------------------------------------

// Wrapper for SHGetSpecialFolderPath
bool getSpecialFolderPath(int index, LPTSTR path) {
	return toBool(SHGetSpecialFolderPath(NULL, path, index, TRUE));
}


// Get the target of a file:
// If the file is a shortcut, return its target; handle all known cases:
// - installer shortcuts (MSI API)
// - IUniformResourceLocator shortcuts
// - IShellLink shortcuts
// If the file is a normal file, return its path unchanged
bool getShellLinkTarget(LPCTSTR link_file, LPTSTR target_path) {
	*target_path = _T('\0');
	
	// Resolve Installer shortcuts
	// Use MSI API dynamically, because it is available only under NT-like
	const HMODULE hMSI = LoadLibrary(_T("Msi.dll"));
	if (hMSI) {
		typedef UINT (WINAPI *PFN_MsiGetShortcutTarget)(
			LPCTSTR szShortcutTarget, LPTSTR szProductCode, LPTSTR szFeatureId, LPTSTR szComponentCode);
		const PFN_MsiGetShortcutTarget pfnMsiGetShortcutTarget =
			myGetProcAddress(hMSI, MsiGetShortcutTarget);
		
		TCHAR pszProductCode[39];
		TCHAR pszComponentCode[39];
		if (pfnMsiGetShortcutTarget && ERROR_SUCCESS ==
				pfnMsiGetShortcutTarget(link_file, pszProductCode, NULL, pszComponentCode)) {
			typedef UINT (WINAPI *PFN_MsiGetComponentPath)(
				LPCTSTR szProduct, LPCTSTR szComponent, LPTSTR lpPathBuf, DWORD* pcchBuf);
			const PFN_MsiGetComponentPath pfnMsiGetComponentPath =
				myGetProcAddress(hMSI, MsiGetComponentPath);
			DWORD buf = MAX_PATH;
			if (pfnMsiGetComponentPath) {
				pfnMsiGetComponentPath(pszProductCode, pszComponentCode, target_path, &buf);
			}
		}
		
		FreeLibrary(hMSI);
		
		if (*target_path) {
			return true;
		}
	}
	
	strToW(MAX_PATH, wszLinkFile, link_file);
	
	// Resolve the shortcut using the IUniformResourceLocator and IPersistFile interfaces
	IUniformResourceLocator *purl;
	if (SUCCEEDED(CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
			IID_IUniformResourceLocator, (LPVOID*)&purl))) {
		IPersistFile *ppf;
		if (SUCCEEDED(purl->QueryInterface(IID_IPersistFile, (void**)&ppf))) {
			LPTSTR pszURL;
			HRESULT hr1, hr2;
			if (SUCCEEDED(hr1 = ppf->Load(wszLinkFile, STGM_READ)) &&
					SUCCEEDED(hr2 = purl->GetURL(&pszURL))) {
				lstrcpyn(target_path, pszURL, MAX_PATH);
				CoTaskMemFree(pszURL);
			}
			ppf->Release();
		}
		purl->Release();
		
		if (*target_path) {
			return true;
		}
	}
	
	// Resolve the shortcut using the IShellLink and IPersistFile interfaces
	IShellLink *psl;
	if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
			IID_IShellLink, (LPVOID*)&psl))) {
		IPersistFile *ppf;
		if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf))) {
			if (SUCCEEDED(ppf->Load(wszLinkFile, STGM_READ)) &&
					SUCCEEDED(psl->Resolve(NULL, MAKELONG(SLR_NO_UI | SLR_NOUPDATE, 1000)))) {
				psl->GetPath(target_path, MAX_PATH, NULL, 0);
			}
			ppf->Release();
		}
		psl->Release();
		
		if (*target_path) {
			return true;
		}
	}
	
	return false;
}


//------------------------------------------------------------------------
// Command line parsing and executing
//------------------------------------------------------------------------

void findFullPath(LPTSTR path, LPTSTR full_path) {
	if (!pathIsSlow(path)) {
		PathUnquoteSpaces(path);
		if (SearchPath(NULL, path, NULL, MAX_PATH, full_path, NULL)) {
			return;
		}
		
		DWORD buf = MAX_PATH;
		if (SUCCEEDED(AssocQueryString(ASSOCF_OPEN_BYEXENAME, ASSOCSTR_EXECUTABLE,
				path, _T("open"), full_path, &buf))) {
			return;
		}
		
		if (32 < reinterpret_cast<UINT_PTR>(FindExecutable(path, NULL, full_path))) {
			return;
		}
	}
	
	lstrcpy(full_path, path);
}


void shellExecuteCmdLine(LPCTSTR command, LPCTSTR directory, int show_mode) {
	// Expand the environment variables before splitting
	TCHAR pszCommandExp[MAX_PATH + bufClipboardString];
	ExpandEnvironmentStrings(command, pszCommandExp, arrayLength(pszCommandExp));
	
	// Split the command line: get the file and the arguments
	TCHAR path[MAX_PATH];
	lstrcpyn(path, pszCommandExp, arrayLength(path));
	PathRemoveArgs(path);
	
	// Get the directory
	TCHAR pszDirectoryBuf[MAX_PATH];
	if (!directory || !*directory) {
		findFullPath(path, pszDirectoryBuf);
		PathRemoveFileSpec(pszDirectoryBuf);
		directory = pszDirectoryBuf;
		PathQuoteSpaces(path);
	}
	
	// Run the command line
	SHELLEXECUTEINFO sei;
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_FLAG_DDEWAIT;
	sei.hwnd = e_hwndInvisible;
	sei.lpFile = path;
	sei.lpVerb = NULL;
	sei.lpParameters = PathGetArgs(pszCommandExp);
	sei.lpDirectory = directory;
	sei.nShow = show_mode;
	ShellExecuteEx(&sei);
}


DWORD WINAPI threadShellExecute(void* params) {
	THREAD_SHELLEXECUTE* const params_ptr = reinterpret_cast<THREAD_SHELLEXECUTE*>(params);
	shellExecuteCmdLine(params_ptr->command, params_ptr->directory, params_ptr->show_mode);
	delete params_ptr;
	return 0;
}


void skipUntilComma(TCHAR*& chr_ptr, bool unescape) {
	TCHAR* const start = chr_ptr;
	bool escaping = false;
	TCHAR* current = start;
	while (*current) {
		if (escaping) {
			escaping = false;
		} else if (*current == _T(',')) {
			break;
		} else if (unescape && *current == _T('\\')) {
			escaping = true;
		}
		current++;
	}
	if (*current) {
		*current++ = _T('\0');
		while (*current == _T(' ')) {
			current++;
		}
	}
	chr_ptr = current;
	
	if (unescape) {
		const TCHAR* input = start;
		TCHAR* output = start;
		bool escaping = false;
		while (*input) {
			if (*input == _T('\\') && !escaping) {
				escaping = true;
			} else{
				escaping = false;
				*output++ = *input;
			}
			input++;
		}
		*output = _T('\0');
	}
}
