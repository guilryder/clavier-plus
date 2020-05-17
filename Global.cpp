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


#include "StdAfx.h"
#include "Global.h"
#include "Com.h"
#include "Shortcut.h"

#include <intshcut.h>
#include <msi.h>
#include <tlhelp32.h>
#include <psapi.h>

#include <dlgs.h>
#undef psh1
#undef psh2

const int kWindowClassBufferLength = 200;

HANDLE e_hHeap;
HINSTANCE e_hInst;
HWND e_hwndInvisible;
HWND e_hdlgModal;
bool e_bIconVisible = true;


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
	SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(link));
	s_prcLabel = SubclassWindow(hwnd, prcWebLink);
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
			ShellExecute(hwnd, NULL, reinterpret_cast<LPCTSTR>(GetWindowLongPtr(hwnd, GWLP_USERDATA)),
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
	
	if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(NULL)) {
		const HGLOBAL clipboard_mem = static_cast<HGLOBAL>(GetClipboardData(CF_UNICODETEXT));
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
	FIND_WINDOW_BY_NAME fwbn;
	fwbn.title_regexp = title_regexp;
	fwbn.hwnd_found = NULL;
	EnumWindows(prcEnumFindWindowByName, reinterpret_cast<LPARAM>(&fwbn));
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


void setClipboardText(LPCTSTR text) {
	if (!OpenClipboard(NULL)) {
		return;
	}
	EmptyClipboard();
	
	// Allocate and fill a global buffer.
	const HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (lstrlen(text) + 1) * sizeof(TCHAR));
	const LPTSTR mem_text = reinterpret_cast<LPTSTR>(GlobalLock(hMem));
	lstrcpy(mem_text, text);
	GlobalUnlock(hMem);
	
	// Pass the buffer to the clipboard.
	SetClipboardData(CF_UNICODETEXT, hMem);
	CloseClipboard();
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
	CoBuffer<LPITEMIDLIST> pidl(SHBrowseForFolder(&bi));
	return pidl && SHGetPathFromIDList(pidl, directory);
}

int CALLBACK prcBrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM, LPARAM lpData) {
	if (uMsg == BFFM_INITIALIZED) {
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}
	return 0;
}


//------------------------------------------------------------------------
// Changes the current directory in the given dialog box.
// Supported dialog boxes:
// - Standard Windows File/Open and similar: GetOpenFileName, GetSaveFileName
// - MS Office File/Open and similar
// - Standard Windows folder selection: SHBrowseForFolder
//------------------------------------------------------------------------

// Helper class used by setDialogBoxDirectory().
// Should be instantiated every time setDialogBoxDirectory() is called.
class SetDialogBoxDirectory {
public:
	
	// Changes the current directory in the given dialog box.
	//
	// Args:
	//   hwnd: The window handle of the dialog box or one of its controls.
	//   directory: The directory to set in the dialog box.
	//
	// Returns:
	//   True if the dialog box is supported and the current directory has been changed.
	bool doit(HWND hwnd, LPCTSTR directory);
	
private:
	
	// One method per supported dialog box.
	// Sets m_success to true if the directory has been successfully changed.
	//
	// Returns:
	//   True on success, false to continue to the next method.
	bool tryBrowseForFolder();
	bool tryMsOfficeFileOpen();
	bool tryWindowsFileOpen();
	
	void lockWindowUpdate() {
		m_unlock_window_update_needed = true;
		LockWindowUpdate(m_hwnd);
	}
	
	// The dialog box handle, is never a control.
	HWND m_hwnd;
	
	// Directory to set the dialog box to, without any quotes.
	TCHAR m_directory_no_quotes[MAX_PATH];
	
	// Set to true by the set*() methods if LockWindowUpdate(NULL) should be called before returning.
	bool m_unlock_window_update_needed;
};


bool setDialogBoxDirectory(HWND hwnd, LPCTSTR directory) {
	return SetDialogBoxDirectory().doit(hwnd, directory);
}


bool SetDialogBoxDirectory::doit(HWND hwnd, LPCTSTR directory) {
	
	// Retrieves the dialog box handle, supports the case where hwnd is a control.
	while (hwnd && (GetWindowStyle(hwnd) & WS_CHILD)) {
		hwnd = GetParent(hwnd);
	}
	if (!hwnd) {
		return false;
	}
	m_hwnd = hwnd;
	
	lstrcpyn(m_directory_no_quotes, directory, arrayLength(m_directory_no_quotes));
	PathUnquoteSpaces(m_directory_no_quotes);
	
	m_unlock_window_update_needed = false;
	
	// Try all supported dialog boxes until one method returns false.
	bool success = tryBrowseForFolder() || tryMsOfficeFileOpen() || tryWindowsFileOpen();
	
	if (m_unlock_window_update_needed) {
		LockWindowUpdate(NULL);
	}
	
	return success;
}


bool SetDialogBoxDirectory::tryBrowseForFolder() {
	
	// Check for SHBrowseForFolder dialog box:
	// - Must contain a control having a class name containing "SHBrowseForFolder"
	if (!findVisibleChildWindow(m_hwnd, _T("SHBrowseForFolder"), true)) {
		return false;
	}
	
	// Send a BFFM_SETSELECTION message to the dialog box to change the current directory.
	// The messages requires the directory string to be a valid string in the dialog box process:
	// we need to use VirtualAllocEx() and WriteProcessMemory().
	
	// 1) Retrieve a handle to the dialog box process
	DWORD process_id;
	GetWindowThreadProcessId(m_hwnd, &process_id);
	const HANDLE process_handle = OpenProcess(
			PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, process_id);
	if (!process_handle) {
		return false;
	}
	
	// 2) Allocate a buffer in the process space for the directory path
	const DWORD size = (lstrlen(m_directory_no_quotes) + 1) * sizeof(TCHAR);
	void *const remote_buf = VirtualAllocEx(process_handle, NULL, size, MEM_COMMIT, PAGE_READWRITE);
	bool success = false;
	if (remote_buf) {
		
		// 3) Fill the buffer
		WriteProcessMemory(process_handle, remote_buf, m_directory_no_quotes, size, NULL);
		
		// 4) Send the message using the buffer
		SendMessage(m_hwnd, BFFM_SETSELECTION, TRUE, reinterpret_cast<LPARAM>(remote_buf));
		
		// 5) Free the buffer
		VirtualFreeEx(process_handle, remote_buf, 0, MEM_RELEASE);
		success = true;
	}
	CloseHandle(process_handle);
	return success;
}


bool SetDialogBoxDirectory::tryMsOfficeFileOpen() {
	
	// Check for MS Office File/Open dialog box
	// - Check that the dialog box class contains "bosa_sdm_"
	// - Find the first visible edit box (Edit or RichEdit)
	if (!checkWindowClass(m_hwnd, _T("bosa_sdm_"), true)) {
		return false;
	}
	
	// The file path edit box control ID depends on MS Office version
	const DWORD style = GetWindowStyle(m_hwnd);
	UINT id = 0;
	if (style == 0x96CC0000) {
		id = 0x36;
	} else if (style == 0x94C80000) {
		id = 0x30;
	}
	
	const HWND hwnd_control = GetDlgItem(m_hwnd, id);
	if (!(id && hwnd_control &&
			(checkWindowClass(hwnd_control, _T("Edit"), false) ||
			checkWindowClass(hwnd_control, _T("RichEdit20W"), false)))) {
		return false;
	}
	
	lockWindowUpdate();
	
	// The control is an edit box: save its contents to path_save, then set it to the directory.
	TCHAR path_save[MAX_PATH];
	SendMessage(hwnd_control, WM_GETTEXT, arrayLength(path_save),
			reinterpret_cast<LPARAM>(path_save));
	lockWindowUpdate();
	if (!SendMessage(hwnd_control, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(m_directory_no_quotes))) {
		return false;
	}
	
	// Simulate a press to ENTER to switch to the new directory and wait a bit.
	PostMessage(hwnd_control, WM_KEYDOWN, VK_RETURN, 0);
	PostMessage(hwnd_control, WM_KEYUP, VK_RETURN, 0);
	sleepBackground(100);
	
	// Restore the saved path.
	SendMessage(hwnd_control, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(path_save));
	return true;
}


bool SetDialogBoxDirectory::tryWindowsFileOpen() {
	
	// Check for standard Get(Open|Save)FileName dialog box
	// - Must answer to CDM_GETSPEC message
	if (SendMessage(m_hwnd, CDM_GETSPEC, 0, NULL) <= 0) {
		return false;
	}
	
	// Two possible controls: file path edit box or combo box
	static const UINT control_ids[] = { cmb13, edt1 };
	for (size_t control = 0; control < arrayLength(control_ids); control++) {
		const HWND hwnd_control = GetDlgItem(m_hwnd, control_ids[control]);
		if (!hwnd_control) {
			continue;
		}
		
		// The control is valid: save its contents to path_save, then set it to the directory.
		TCHAR path_save[MAX_PATH];
		SendMessage(hwnd_control, WM_GETTEXT, arrayLength(path_save),
				reinterpret_cast<LPARAM>(path_save));
		lockWindowUpdate();
		if (SendMessage(hwnd_control, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(m_directory_no_quotes))) {
			
			// Simulate a click on the OK button.
			sleepBackground(0);
			SendMessage(m_hwnd, WM_COMMAND, IDOK, 0);
			sleepBackground(0);
			
			// Restore the saved path.
			SendMessage(hwnd_control, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(path_save));
			return true;
		}
	}
	
	return false;
}


//------------------------------------------------------------------------
// Shell API tools
//------------------------------------------------------------------------

// Wrapper for SHGetSpecialFolderPath
bool getSpecialFolderPath(int csidl, LPTSTR path) {
	return toBool(SHGetSpecialFolderPath(NULL, path, csidl, TRUE));
}


// Populates a shortcut with the target of a link file.
// Handles special cases natively:
// - installer shortcuts (MSI API)
// - IUniformResourceLocator shortcuts
// - IShellLink shortcuts
// Does not natively handle shortcuts to UWP apps.
void resolveLinkFile(LPCTSTR link_file, Shortcut* shortcut) {
	shortcut->m_bCommand = true;
	String& cmdline = shortcut->m_sCommand;
	
	TCHAR pszProductCode[39];
	TCHAR pszComponentCode[39];
	if (!MsiGetShortcutTarget(link_file, pszProductCode, NULL, pszComponentCode)) {
		DWORD buf = MAX_PATH;
		MsiGetComponentPath(pszProductCode, pszComponentCode, cmdline.getBuffer(buf), &buf);
		if (cmdline.isSome()) {
			PathQuoteSpaces(cmdline.get());
			return;
		}
	}
	
	// Resolve the shortcut using the IUniformResourceLocator and IPersistFile interfaces
	CoPtr<IUniformResourceLocator> url_locator(CLSID_InternetShortcut);
	if (url_locator) {
		auto persist_file = url_locator.queryInterface<IPersistFile>();
		if (persist_file) {
			CoBuffer<LPTSTR> url;
			if (SUCCEEDED(persist_file->Load(link_file, STGM_READ)) &&
					SUCCEEDED(url_locator->GetURL(&url)) &&
					url && *url) {
				cmdline = url;
				return;
			}
		}
	}
	
	// Resolve the shortcut using the IShellLink and IPersistFile interfaces
	CoPtr<IShellLink> shell_link(CLSID_ShellLink);
	if (shell_link) {
		auto persist_file = shell_link.queryInterface<IPersistFile>();
		if (persist_file &&
				SUCCEEDED(persist_file->Load(link_file, STGM_READ)) &&
				SUCCEEDED(shell_link->Resolve(nullptr, MAKELONG(SLR_NO_UI | SLR_NOUPDATE, 1000))) &&
				SUCCEEDED(shell_link->GetPath(cmdline.getBuffer(MAX_PATH), MAX_PATH, nullptr, SLGP_RAWPATH))) {
			PathQuoteSpaces(cmdline.get());
			
			TCHAR args[MAX_PATH];
			if (SUCCEEDED(shell_link->GetArguments(args, arrayLength(args))) && args[0]) {
				cmdline += _T(' ');
				cmdline += args;
			}
			if (cmdline.isSome()) {
				shell_link->GetWorkingDirectory(shortcut->m_sDirectory.getBuffer(MAX_PATH), MAX_PATH);
				int show;
				if (SUCCEEDED(shell_link->GetShowCmd(&show))) {
					shortcut->m_nShow = show;
				}
				return;
			}
		}
	}
	
	// Unsupported link file type: use its path as-is.
	cmdline = link_file;
	PathQuoteSpaces(cmdline.getBuffer(MAX_PATH));
}


//------------------------------------------------------------------------
// Strings
//------------------------------------------------------------------------

LPCTSTR getSemiColonToken(LPTSTR& token_start) {
	const LPTSTR token_start_copy = token_start;
	LPTSTR current = token_start_copy;
	while (*current != _T(';')) {
		if (!*current) {
			token_start = current;
			return token_start_copy;
		}
		current++;
	}
	*current = _T('\0');
	token_start = current + 1;
	return token_start_copy;
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
	TCHAR command_exp[MAX_PATH + bufClipboardString];
	ExpandEnvironmentStrings(command, command_exp, arrayLength(command_exp));
	
	// Split the command line: get the file and the arguments
	TCHAR path[MAX_PATH];
	lstrcpyn(path, command_exp, arrayLength(path));
	PathRemoveArgs(path);
	
	// If the directory is empty, get it from the file
	TCHAR real_directory[MAX_PATH];
	if (!directory || !*directory) {
		findFullPath(path, real_directory);
		PathRemoveFileSpec(real_directory);
		PathQuoteSpaces(path);
	} else {
		ExpandEnvironmentStrings(directory, real_directory, arrayLength(real_directory));
	}
	
	// Run the command line
	SHELLEXECUTEINFO sei;
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_FLAG_DDEWAIT;
	sei.hwnd = e_hwndInvisible;
	sei.lpFile = path;
	sei.lpVerb = NULL;
	sei.lpParameters = PathGetArgs(command_exp);
	sei.lpDirectory = real_directory;
	sei.nShow = show_mode;
	ShellExecuteEx(&sei);
}


DWORD WINAPI ShellExecuteThread::thread(void* params) {
	ShellExecuteThread* const params_ptr = reinterpret_cast<ShellExecuteThread*>(params);
	shellExecuteCmdLine(params_ptr->m_command, params_ptr->m_directory, params_ptr->m_show_mode);
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
	
	if (!unescape) {
		return;
	}
	const TCHAR* input = start;
	TCHAR* output = start;
	escaping = false;
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


//------------------------------------------------------------------------
// Autostart setting
// Persisted in the Registry under the key autostart_key, in a value
// named autostart_value.
//------------------------------------------------------------------------

static const TCHAR autostart_key[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
static const LPCTSTR autostart_value = pszApp;

static HKEY openAutoStartKey(LPTSTR pszPath);

// Open the Registry key for the auto-start setting.
//
// Also stores in autostart_path_buf the string to store under the key for the value named
// autostart_value to enable auto-start, that is the path of the Clavier+ executable.
// Expected to have a capacity >= MAX_PATH, set to empty string on error.
//
// Returns the opened key, NULL on error.
// The caller is responsible for closing the key with RegCloseKey().
HKEY openAutoStartKey(LPTSTR autostart_path_buf) {
	if (!GetModuleFileName(NULL, autostart_path_buf, MAX_PATH)) {
		*autostart_path_buf = _T('\0');
	}
	HKEY hKey;
	if (ERROR_SUCCESS != RegOpenKey(HKEY_CURRENT_USER, autostart_key, &hKey)) {
		hKey = NULL;
	}
	return hKey;
}

bool isAutoStartEnabled() {
	bool enabled = false;
	
	TCHAR autostart_path_buf[MAX_PATH];
	const HKEY autostart_hKey = openAutoStartKey(autostart_path_buf);
	if (!autostart_hKey) {
		return enabled;
	}
	
	DWORD type, size;
	TCHAR current_path[MAX_PATH];
	
	if (ERROR_SUCCESS == RegQueryValueEx(autostart_hKey, autostart_value, NULL,
			&type, NULL, &size) && type == REG_SZ && size < sizeof(current_path)) {
		if (ERROR_SUCCESS == RegQueryValueEx(autostart_hKey, autostart_value, NULL,
					NULL, (BYTE*)current_path, &size) &&
				!lstrcmpi(current_path, autostart_path_buf)) {
			enabled = true;
		}
	}
	
	RegCloseKey(autostart_hKey);
	return enabled;
}

void setAutoStartEnabled(bool enabled) {
	TCHAR autostart_path_buf[MAX_PATH];
	const HKEY autostart_hKey = openAutoStartKey(autostart_path_buf);
	if (!autostart_hKey) {
		return;
	}
	
	if (enabled) {
		RegSetValueEx(autostart_hKey, autostart_value, NULL, REG_SZ,
				(const BYTE*)autostart_path_buf, (lstrlen(autostart_path_buf) + 1) * sizeof(TCHAR));
	} else {
		RegDeleteValue(autostart_hKey, autostart_value);
	}
	RegCloseKey(autostart_hKey);
}
