// Clavier+
// Keyboard shortcuts manager
//
// Copyright (C) 2000-2008 Guillaume Ryder
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


#include "StdAfx.h"
#include "Global.h"

#include <intshcut.h>
#include <tlhelp32.h>

HANDLE e_hHeap;
HINSTANCE e_hInst;
HWND e_hwndInvisible;
HWND e_hdlgModal;
bool e_bIconVisible = true;

#define myGetProcAddress(hDLL, functionName) \
	((PFN_##functionName)GetProcAddress(hDLL, (#functionName ANSI_UNICODE("A", "W"))))


static bool pathIsSlow(LPCTSTR pszPath);

static BOOL CALLBACK prcEnumFindWindowByName(HWND hWnd, LPARAM lParam);


int messageBox(HWND hWnd, UINT idString, UINT uType, LPCTSTR pszArg) {
	TCHAR pszFormat[256], pszText[1024];
	loadStringAuto(idString, pszFormat);
	wsprintf(pszText, pszFormat, pszArg);
	return MessageBox(hWnd, pszText, pszApp, uType);
}


void centerParent(HWND hWnd) {
	RECT rcParent, rcChild;
	
	const HWND hwndParent = GetParent(hWnd);
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	if (hwndParent) {
		const HMONITOR hMonitor = MonitorFromWindow(hwndParent, MONITOR_DEFAULTTONULL);
		VERIFV(hMonitor);
		GetMonitorInfo(hMonitor, &mi);
		GetWindowRect(hwndParent, &rcParent);
		IntersectRect(&rcParent, &rcParent, &mi.rcWork);
	} else {
		const HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		VERIFV(hMonitor);
		GetMonitorInfo(hMonitor, &mi);
		rcParent = mi.rcWork;
	}
	
	GetWindowRect(hWnd, &rcChild);
	SetWindowPos(hWnd, NULL,
		(rcParent.left + rcParent.right  + rcChild.left - rcChild.right)  / 2,
		(rcParent.top  + rcParent.bottom + rcChild.top  - rcChild.bottom) / 2,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}


void getDlgItemText(HWND hDlg, UINT id, String& rs) {
	const HWND hwnd = GetDlgItem(hDlg, id);
	const int buf = GetWindowTextLength(hwnd) + 1;
	GetWindowText(hwnd, rs.getBuffer(buf), buf);
}


static WNDPROC s_prcLabel;
static LRESULT CALLBACK prcWebLink(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void initializeWebLink(HWND hDlg, UINT idControl, LPCTSTR pszLink) {
	const HWND hwnd = GetDlgItem(hDlg, idControl);
	SetWindowLong(hwnd, GWL_USERDATA, (LONG)pszLink);
	s_prcLabel = SubclassWindow(hwnd, prcWebLink);
}

// Window procedure for dialog box controls displaying an URL.
// The link itself is stored in GWL_USERDATA.
LRESULT CALLBACK prcWebLink(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		
		case WM_SETCURSOR:
			SetCursor(LoadCursor(NULL, IDC_HAND));
			return TRUE;
		
		case WM_NCHITTEST:
			return HTCLIENT;
		
		case WM_LBUTTONDOWN:
			ShellExecute(hWnd, NULL, (LPCTSTR)GetWindowLong(hWnd, GWL_USERDATA), NULL, NULL, SW_SHOWDEFAULT);
			return 0;
	}
	
	return CallWindowProc(s_prcLabel, hWnd, uMsg, wParam, lParam);
}


// Wrapper for CreateThread().
void startThread(LPTHREAD_START_ROUTINE pfn, void* pParams) {
	DWORD idThread;
	CloseHandle(CreateThread(NULL, 0, pfn, pParams, 0, &idThread));
}


void writeFile(HANDLE hf, LPCTSTR psz) {
	DWORD len;
	WriteFile(hf, psz, lstrlen(psz) * sizeof(TCHAR), &len, NULL);
}


bool getWindowExecutable(HWND hWnd, LPTSTR pszExecutableName) {
	DWORD idProcess;
	GetWindowThreadProcessId(hWnd, &idProcess);
	
	typedef DWORD (WINAPI* PFN_GetProcessImageFileName)(
		HANDLE hProcess, LPTSTR lpImageFileName, DWORD nSize);
	
	static bool bTryLoadPSAPI = true;
	static PFN_GetProcessImageFileName pfnGetProcessImageFileName = NULL;
	
	if (!pfnGetProcessImageFileName && bTryLoadPSAPI) {
		// Try to load NT function
		
		bTryLoadPSAPI = false;
		const HMODULE hPSAPI = LoadLibrary(_T("PSAPI.dll"));
		if (hPSAPI) {
			pfnGetProcessImageFileName = myGetProcAddress(hPSAPI, GetProcessImageFileName);
			if (!pfnGetProcessImageFileName) {
				FreeLibrary(hPSAPI);
			}
		}
	}
	
	bool bOK = false;
	if (pfnGetProcessImageFileName) {
		// Use NT function
		
		const HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, idProcess);
		VERIF(hProcess);
		bOK = (pfnGetProcessImageFileName(hProcess, pszExecutableName, MAX_PATH) > 0);
		CloseHandle(hProcess);
		
	} else {
		// Use Win 9x helper functions
		const HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		VERIF(hSnapshot != INVALID_HANDLE_VALUE);
		PROCESSENTRY32 pe;
		pe.dwSize = sizeof(pe);
		if (Process32First(hSnapshot, &pe)) {
			do {
				if (pe.th32ProcessID == idProcess) {
					bOK = true;
					lstrcpy(pszExecutableName, pe.szExeFile);
					PathStripPath(pszExecutableName);
					break;
				}
			} while (Process32Next(hSnapshot, &pe));
		}
		CloseHandle(hSnapshot);
	}
	
	if (bOK) {
		PathStripPath(pszExecutableName);
		CharLower(pszExecutableName);
	}
	
	return bOK;
}


void sleepBackground(DWORD dwDurationMS) {
	const HANDLE hProcess = GetCurrentProcess();
	const DWORD dwOldPriority = GetPriorityClass(hProcess);
	SetPriorityClass(hProcess, IDLE_PRIORITY_CLASS);
	Sleep(dwDurationMS);
	SetPriorityClass(hProcess, dwOldPriority);
}


// Indicates if a file is located in a slow drive (network, removable, etc.).
bool pathIsSlow(LPCTSTR pszPath) {
	const int iDrive = PathGetDriveNumber(pszPath);
	if (iDrive >= 0) {
		TCHAR pszRoot[4];
		PathBuildRoot(pszRoot, iDrive);
		switch (GetDriveType(pszRoot)) {
			case DRIVE_UNKNOWN:
			case DRIVE_REMOVABLE:
			case DRIVE_REMOTE:
			case DRIVE_CDROM:
				return true;
		}
	} else if (pszPath[0] == '\\' && pszPath[1] == '\\') {
		return true;
	}
	return false;
}


bool getFileInfo(LPCTSTR pszPath, DWORD dwFileAttributes, SHFILEINFO& shfi, UINT uFlags) {
	TCHAR pszPathTemp[MAX_PATH];
	
	if (pathIsSlow(pszPath)) {
		// If UNC server or share, remove any trailing backslash
		// If normal file, strip the path: extension-based icon
		
		uFlags |= SHGFI_USEFILEATTRIBUTES;
		lstrcpy(pszPathTemp, pszPath);
		PathRemoveBackslash(pszPathTemp);
		const LPCTSTR pszPathName = PathFindFileName(pszPath);
		if (PathIsUNCServer(pszPathTemp) || PathIsUNCServerShare(pszPathTemp)) {
			pszPath = pszPathTemp;
		} else if (pszPathName) {
			lstrcpy(pszPathTemp, pszPathName);
			pszPath = pszPathTemp;
		}
	}
	
	for (;;) {
		const bool bOK = ToBool(SHGetFileInfo(pszPath, dwFileAttributes,
				&shfi, sizeof(shfi), uFlags));
		if (bOK || (uFlags & SHGFI_USEFILEATTRIBUTES)) {
			return bOK;
		}
		uFlags |= SHGFI_USEFILEATTRIBUTES;
	}
}


void clipboardToEnvironment() {
	bool bOK = false;
	
	if (IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(NULL)) {
		const HGLOBAL hMem = (HGLOBAL)GetClipboardData(CF_TEXT);
		if (hMem) {
			const LPSTR pszClipboard = (LPSTR)GlobalLock(hMem);
			if (pszClipboard) {
				if (lstrlenA(pszClipboard) < bufClipboardString) {
					bOK = true;
					SetEnvironmentVariableA("CLIPBOARD", pszClipboard);
				}
				GlobalUnlock(hMem);
			}
			CloseClipboard();
		}
	}
	
	if (!bOK) {
		SetEnvironmentVariableA("CLIPBOARD", "");
	}
}


HWND findVisibleChildWindow(HWND hwndParent, LPCTSTR pszClass, bool bPrefix) {
	HWND hwndChild = NULL;
	while (ToBool(hwndChild = FindWindowEx(hwndParent, hwndChild, NULL, NULL))) {
		if ((GetWindowStyle(hwndChild) & WS_VISIBLE) &&
				checkWindowClass(hwndChild, pszClass, bPrefix)) {
			return hwndChild;
		}
	}
	return NULL;
}

bool checkWindowClass(HWND hWnd, LPCTSTR pszClass, bool bPrefix) {
	TCHAR pszWindowClass[200];
	VERIF(GetClassName(hWnd, pszWindowClass, nbArray(pszWindowClass)));
	if (bPrefix) {
		pszWindowClass[lstrlen(pszClass)] = _T('\0');
	}
	return !StrCmp(pszWindowClass, pszClass);
}

struct FINDWINDOWBYNAME {
	LPCTSTR pszWindowSpec;
	HWND hwndFound;
};

HWND findWindowByName(LPCTSTR pszWindowSpec) {
	const LPTSTR pszWindowSpecLowercase = new TCHAR[lstrlen(pszWindowSpec) + 1];
	lstrcpy(pszWindowSpecLowercase, pszWindowSpec);
	CharLower(pszWindowSpecLowercase);
	
	FINDWINDOWBYNAME fwbn;
	fwbn.pszWindowSpec = pszWindowSpecLowercase;
	fwbn.hwndFound = NULL;
	EnumWindows(prcEnumFindWindowByName, (LPARAM)&fwbn);
	delete [] pszWindowSpecLowercase;
	return fwbn.hwndFound;
}

BOOL CALLBACK prcEnumFindWindowByName(HWND hWnd, LPARAM lParam) {
	FINDWINDOWBYNAME &fwbn = *(FINDWINDOWBYNAME*)lParam;
	TCHAR pszTitle[1024];
	if (GetWindowText(hWnd, pszTitle, nbArray(pszTitle))) {
		if (matchWildcards(fwbn.pszWindowSpec, pszTitle)) {
			fwbn.hwndFound = hWnd;
			return FALSE;
		}
	}
	return TRUE;
}


bool matchWildcards(LPCTSTR pszPattern, LPCTSTR pszSubject) {
	for (;;) {
		TCHAR c = *pszPattern++;
		switch (c) {
			
			case _T('\0'):
				return !*pszSubject;
			
			case _T('*'):
				if (!*pszPattern) {
					return true;  // Optimization
				}
				do {
					if (matchWildcards(pszPattern, pszSubject)) {
						return true;
					}
				} while (*pszSubject++);
				return false;
			
			case _T('?'):
				VERIF(*pszSubject);
				pszSubject++;
				break;
			
			default:
				VERIF((TCHAR)CharLower((LPTSTR)MAKELONG(*pszSubject++, 0)) == c);
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

bool browseForFolder(HWND hwndParent, LPCTSTR pszTitle, LPTSTR pszDirectory) {
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(bi));
	bi.hwndOwner = hwndParent;
	bi.lpszTitle = pszTitle;
	bi.ulFlags   = BIF_RETURNONLYFSDIRS;
	bi.lpfn      = prcBrowseForFolderCallback;
	bi.lParam    = (LPARAM)pszDirectory;
	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if (!pidl || !SHGetPathFromIDList(pidl, pszDirectory)) {
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
bool getSpecialFolderPath(int index, LPTSTR pszPath) {
	return ToBool(SHGetSpecialFolderPath(NULL, pszPath, index, TRUE));
}


// Get the target of a file:
// If the file is a shortcut, return its target; handle all known cases:
// - installer shortcuts (MSI API)
// - IUniformResourceLocator shortcuts
// - IShellLink shortcuts
// If the file is a normal file, return its path unchanged
bool getShellLinkTarget(LPCTSTR pszLinkFile, LPTSTR pszTargetPath) {
	*pszTargetPath = _T('\0');
	
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
				pfnMsiGetShortcutTarget(pszLinkFile, pszProductCode, NULL, pszComponentCode)) {
			typedef UINT (WINAPI *PFN_MsiGetComponentPath)(
				LPCTSTR szProduct, LPCTSTR szComponent, LPTSTR lpPathBuf, DWORD* pcchBuf);
			const PFN_MsiGetComponentPath pfnMsiGetComponentPath =
				myGetProcAddress(hMSI, MsiGetComponentPath);
			DWORD buf = MAX_PATH;
			if (pfnMsiGetComponentPath) {
				pfnMsiGetComponentPath(pszProductCode, pszComponentCode, pszTargetPath, &buf);
			}
		}
		
		FreeLibrary(hMSI);
		
		if (*pszTargetPath) {
			return true;
		}
	}
	
	strToW(MAX_PATH, wszLinkFile, pszLinkFile);
	
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
				lstrcpyn(pszTargetPath, pszURL, MAX_PATH);
				CoTaskMemFree(pszURL);
			}
			ppf->Release();
		}
		purl->Release();
		
		if (*pszTargetPath) {
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
				psl->GetPath(pszTargetPath, MAX_PATH, NULL, 0);
			}
			ppf->Release();
		}
		psl->Release();
		
		if (*pszTargetPath) {
			return true;
		}
	}
	
	return false;
}


//------------------------------------------------------------------------
// Command line parsing and executing
//------------------------------------------------------------------------

void findFullPath(LPTSTR pszPath, LPTSTR pszFullPath) {
	if (!pathIsSlow(pszPath)) {
		PathUnquoteSpaces(pszPath);
		if (SearchPath(NULL, pszPath, NULL, MAX_PATH, pszFullPath, NULL)) {
			return;
		}
		
		DWORD buf = MAX_PATH;
		if (SUCCEEDED(AssocQueryString(ASSOCF_OPEN_BYEXENAME, ASSOCSTR_EXECUTABLE,
				pszPath, _T("open"), pszFullPath, &buf))) {
			return;
		}
		
		if (32 < (DWORD)FindExecutable(pszPath, NULL, pszFullPath)) {
			return;
		}
	}
	
	lstrcpy(pszFullPath, pszPath);
}


void shellExecuteCmdLine(LPCTSTR pszCommand, LPCTSTR pszDirectory, int nShow) {
	// Expand the environment variables before splitting
	TCHAR pszCommandExp[MAX_PATH + bufClipboardString];
	ExpandEnvironmentStrings(pszCommand, pszCommandExp, nbArray(pszCommandExp));
	
	// Split the command line: get the file and the arguments
	TCHAR pszPath[MAX_PATH];
	lstrcpyn(pszPath, pszCommandExp, nbArray(pszPath));
	PathRemoveArgs(pszPath);
	
	// Get the directory
	TCHAR pszDirectoryBuf[MAX_PATH];
	if (!pszDirectory || !*pszDirectory) {
		findFullPath(pszPath, pszDirectoryBuf);
		PathRemoveFileSpec(pszDirectoryBuf);
		pszDirectory = pszDirectoryBuf;
		PathQuoteSpaces(pszPath);
	}
	
	// Run the command line
	SHELLEXECUTEINFO sei;
	sei.cbSize       = sizeof(sei);
	sei.fMask        = SEE_MASK_FLAG_DDEWAIT;
	sei.hwnd         = e_hwndInvisible;
	sei.lpFile       = pszPath;
	sei.lpVerb       = NULL;
	sei.lpParameters = PathGetArgs(pszCommandExp);
	sei.lpDirectory  = pszDirectory;
	sei.nShow        = nShow;
	ShellExecuteEx(&sei);
}


DWORD WINAPI threadShellExecute(void* pParams) {
	THREAD_SHELLEXECUTE &params = *(THREAD_SHELLEXECUTE*)pParams;
	shellExecuteCmdLine(params.sCommand, params.sDirectory, params.nShow);
	delete (THREAD_SHELLEXECUTE*)pParams;
	return 0;
}
