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

#include <algorithm>
#include <intshcut.h>
#include <msi.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <propsys.h>
#include <propvarutil.h>

#include <dlgs.h>
#undef psh1
#undef psh2

const int kWindowClassBufferLength = 200;

HANDLE e_heap;
HINSTANCE e_instance;
HWND e_invisible_window;
HWND e_modal_dialog;
bool e_icon_visible = true;

int e_column_widths[colCount];

SIZE e_sizeMainDialog = { 0, 0 };
bool e_maximize_main_dialog = false;

TCHAR e_ini_filepath[MAX_PATH];


static bool pathIsSlow(LPCTSTR path);

static BOOL CALLBACK prcEnumFindWindowByName(HWND hwnd, LPARAM lParam);


int messageBox(HWND hwnd, UINT string_id, UINT type, LPCTSTR arg) {
	TCHAR format[256], text[1024];
	i18n::loadStringAuto(string_id, format);
	wsprintf(text, format, arg);
	return MessageBox(hwnd, text, pszApp, type);
}


void centerParent(HWND hwnd) {
	RECT rcParent, rcChild;
	
	const HWND hwnd_parent = GetParent(hwnd);
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	if (hwnd_parent) {
		const HMONITOR monitor = MonitorFromWindow(hwnd_parent, MONITOR_DEFAULTTONULL);
		VERIFV(monitor);
		GetMonitorInfo(monitor, &mi);
		GetWindowRect(hwnd_parent, &rcParent);
		IntersectRect(&rcParent, &rcParent, &mi.rcWork);
	} else {
		const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		VERIFV(monitor);
		GetMonitorInfo(monitor, &mi);
		rcParent = mi.rcWork;
	}
	
	GetWindowRect(hwnd, &rcChild);
	SetWindowPos(
		hwnd, /* hWndInsertAfter= */ NULL,
		(rcParent.left + rcParent.right + rcChild.left - rcChild.right) / 2,
		(rcParent.top + rcParent.bottom + rcChild.top - rcChild.bottom) / 2,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}


void getDlgItemText(HWND hdlg, UINT id, String* output) {
	const HWND hwnd = GetDlgItem(hdlg, id);
	const int buf = GetWindowTextLength(hwnd) + 1;
	GetWindowText(hwnd, output->getBuffer(buf), buf);
}


static WNDPROC s_prcLabel;
static LRESULT CALLBACK prcWebLink(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

void initializeWebLink(HWND hdlg, UINT control_id, LPCTSTR link) {
	const HWND hwnd = GetDlgItem(hdlg, control_id);
	SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(link));
	s_prcLabel = SubclassWindow(hwnd, prcWebLink);
}

// Window procedure for dialog box controls displaying an URL.
// The link itself is stored in GWL_USERDATA.
LRESULT CALLBACK prcWebLink(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_SETCURSOR:
			SetCursor(LoadCursor(/* hInstance= */ NULL, IDC_HAND));
			return true;
		
		case WM_NCHITTEST:
			return HTCLIENT;
		
		case WM_LBUTTONDOWN:
			ShellExecute(
				hwnd,
				/* lpOperation= */ nullptr,
				/* lpFile= */ reinterpret_cast<LPCTSTR>(GetWindowLongPtr(hwnd, GWLP_USERDATA)),
				/* lpParameters= */ nullptr,
				/* lpDirectory= */ nullptr,
				SW_SHOWDEFAULT);
			return 0;
	}
	
	return CallWindowProc(s_prcLabel, hwnd, message, wParam, lParam);
}


// Wrapper for CreateThread().
void startThread(LPTHREAD_START_ROUTINE pfn, void* params) {
	DWORD idThread;
	CloseHandle(CreateThread(
		/* lpThreadAttributes= */ nullptr, /* dwStackSize= */ 0, pfn, params, /* dwCreationFlags= */ 0, &idThread));
}


void writeFile(HANDLE file, LPCTSTR strbuf) {
	DWORD len;
	WriteFile(file, strbuf, lstrlen(strbuf) * sizeof(*strbuf), &len, /* lpOverlapped= */ nullptr);
}


bool getWindowProcessName(HWND hwnd, LPTSTR process_name) {
	DWORD process_id;
	GetWindowThreadProcessId(hwnd, &process_id);
	
	const HANDLE process_handle = OpenProcess(PROCESS_QUERY_INFORMATION, /* bInheritHandle= */ false, process_id);
	VERIF(process_handle);
	const bool ok = (GetProcessImageFileName(process_handle, process_name, MAX_PATH) > 0);
	CloseHandle(process_handle);
	
	if (ok) {
		PathStripPath(process_name);
		CharLower(process_name);
	}
	
	return ok;
}


void sleepBackground(DWORD duration_millis) {
	const HANDLE process = GetCurrentProcess();
	const DWORD old_priority = GetPriorityClass(process);
	SetPriorityClass(process, IDLE_PRIORITY_CLASS);
	Sleep(duration_millis);
	SetPriorityClass(process, old_priority);
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
	
	if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(/* hWndNewOwner= */ NULL)) {
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
	HWND hwnd_child = NULL;
	while (toBool(hwnd_child = FindWindowEx(
			hwnd_parent, hwnd_child, /* lpszClass= nullptr */ nullptr, /* lpszWindow= */ nullptr))) {
		if ((GetWindowStyle(hwnd_child) & WS_VISIBLE) &&
				checkWindowClass(hwnd_child, wnd_class, allow_same_prefix)) {
			return hwnd_child;
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
	FIND_WINDOW_BY_NAME *fwbn = reinterpret_cast<FIND_WINDOW_BY_NAME*>(lParam);
	TCHAR title[1024];
	if (GetWindowText(hwnd, title, arrayLength(title))) {
		if (matchWildcards(fwbn->title_regexp, title)) {
			fwbn->hwnd_found = hwnd;
			return false;
		}
	}
	return true;
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
	if (!OpenClipboard(/* hWndNewOwner= */ NULL)) {
		return;
	}
	EmptyClipboard();
	
	// Allocate and fill a global buffer.
	const HGLOBAL buffer = GlobalAlloc(GMEM_MOVEABLE, (lstrlen(text) + 1) * sizeof(*text));
	const LPTSTR locked_buffer = reinterpret_cast<LPTSTR>(GlobalLock(buffer));
	lstrcpy(locked_buffer, text);
	GlobalUnlock(buffer);
	
	// Pass the buffer to the clipboard.
	SetClipboardData(CF_UNICODETEXT, buffer);
	CloseClipboard();
}


//------------------------------------------------------------------------
// SHBrowseForFolder wrapper:
// - use given title
// - set initial directory
// - return directory path instead of LPITEMIDLIST
//------------------------------------------------------------------------

static int CALLBACK prcBrowseForFolderCallback(HWND hwnd, UINT message, LPARAM lParam, LPARAM lpData);

bool browseForFolder(HWND hwnd_parent, LPCTSTR title, LPTSTR directory) {
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(bi));
	bi.hwndOwner = hwnd_parent;
	bi.lpszTitle = title;
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = prcBrowseForFolderCallback;
	bi.lParam = reinterpret_cast<LPARAM>(directory);
	CoBuffer<LPITEMIDLIST> pidl(SHBrowseForFolder(&bi));
	return pidl && SHGetPathFromIDList(pidl, directory);
}

int CALLBACK prcBrowseForFolderCallback(HWND hwnd, UINT message, LPARAM UNUSED(lParam), LPARAM lpData) {
	if (message == BFFM_INITIALIZED) {
		SendMessage(hwnd, BFFM_SETSELECTION, true, lpData);
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
	// Return true if the dialog box is supported and the current directory has been changed.
	//
	// hwnd: the window handle of the dialog box or one of its controls.
	// directory: the directory to set in the dialog box.
	bool doit(HWND hwnd, LPCTSTR directory);
	
private:
	
	// One method per supported dialog box.
	// Sets m_success to true if the directory has been successfully changed.
	// Returns true on success, false to continue to the next method.
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
		PROCESS_VM_OPERATION | PROCESS_VM_WRITE, /* bInheritHandle= */ false, process_id);
	if (!process_handle) {
		return false;
	}
	
	// 2) Allocate a buffer in the process space for the directory path
	const DWORD size = (lstrlen(m_directory_no_quotes) + 1) * sizeof(*m_directory_no_quotes);
	void *const remote_buf =
		VirtualAllocEx(process_handle, /* lpAddress= */ nullptr, size, MEM_COMMIT, PAGE_READWRITE);
	bool success = false;
	if (remote_buf) {
		
		// 3) Fill the buffer
		WriteProcessMemory(
			process_handle, remote_buf, m_directory_no_quotes, size,
			/* lpNumberOfBytesWritten= */ nullptr);
		
		// 4) Send the message using the buffer
		SendMessage(m_hwnd, BFFM_SETSELECTION, true, reinterpret_cast<LPARAM>(remote_buf));
		
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
	if (SendMessage(m_hwnd, CDM_GETSPEC, 0, /* buffer= */ NULL) <= 0) {
		return false;
	}
	
	// Two possible controls: file path edit box or combo box
	static constexpr UINT control_ids[] = { cmb13, edt1 };
	for (size_t control = 0; control < arrayLength(control_ids); control++) {
		const HWND hwnd_control = GetDlgItem(m_hwnd, control_ids[control]);
		if (!hwnd_control) {
			continue;
		}
		
		// The control is valid: save its contents to path_save, then set it to the directory.
		TCHAR path_save[MAX_PATH];
		SendMessage(hwnd_control, WM_GETTEXT, arrayLength(path_save), reinterpret_cast<LPARAM>(path_save));
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
	return toBool(SHGetSpecialFolderPath(/* hWnd= */ NULL, path, csidl, /* fCreate= */ true));
}


// Populates a shortcut with the target of a link file.
// Handles special cases natively:
// - installer shortcuts (MSI API)
// - IUniformResourceLocator shortcuts
// - IShellLink shortcuts
// Does not natively handle shortcuts to UWP apps.
void resolveLinkFile(LPCTSTR link_file, Shortcut* shortcut) {
	shortcut->m_type = Shortcut::Type::Command;
	String& cmdline = shortcut->m_command;
	
	TCHAR product_code[39];
	TCHAR component_code[39];
	if (!MsiGetShortcutTarget(link_file, product_code, /* szFeatureId= */ nullptr, component_code)) {
		DWORD buf = MAX_PATH;
		MsiGetComponentPath(product_code, component_code, cmdline.getBuffer(buf), &buf);
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
				shell_link->GetWorkingDirectory(shortcut->m_directory.getBuffer(MAX_PATH), MAX_PATH);
				int show;
				if (SUCCEEDED(shell_link->GetShowCmd(&show))) {
					shortcut->m_show_option = show;
				}
				return;
			}
		}
	}
	
	// Unsupported link file type: use its path as-is.
	cmdline = link_file;
	PathQuoteSpaces(cmdline.getBuffer(MAX_PATH));
}


void listUwpApps(std::function<void(LPCTSTR name, LPCTSTR app_id, LPITEMIDLIST pidl)> app_callback) {
	constexpr ULONG kMaxAppItemCount = 100;
	constexpr size_t kMaxAppIdLength = 1000;
	
	// Retrieve a folder manager.
	CoPtr<IKnownFolderManager> manager(CLSID_KnownFolderManager);
	VERIFV(manager);
	
	// Retrieve the apps folder.
	CoPtr<IKnownFolder> known_folder;
	CoPtr<IShellItem> folder_item;
	VERIFV(SUCCEEDED(manager->GetFolder(FOLDERID_AppsFolder, &known_folder)) &&
		SUCCEEDED(known_folder->GetShellItem(KF_FLAG_DEFAULT, IID_PPV_ARGS(&folder_item))));
	
	// Property set only on non-UWP apps.
	PROPERTYKEY non_uwp_prop_key;
	VERIFV(SUCCEEDED(PSGetPropertyKeyFromName(_T("System.Link.TargetParsingPath"), &non_uwp_prop_key)));
	
	// Contains the app ID for UWP apps.
	PROPERTYKEY id_prop_key;
	VERIFV(SUCCEEDED(PSGetPropertyKeyFromName(_T("System.AppUserModel.ID"), &id_prop_key)));
	
	// Enumerate the apps folder children, UWP and non-UWP apps alike.
	CoPtr<IEnumShellItems> enum_apps;
	VERIFV(SUCCEEDED(folder_item->BindToHandler(nullptr, BHID_StorageEnum, IID_PPV_ARGS(&enum_apps))));
	
	// Collect the first kMaxAppItemCount UWP apps.
	struct UwpApp {
		CoPtr<IShellItem> item;
		CoPtr<IPropertyStore> props;
	};
	UwpApp uwp_apps[kMaxAppItemCount];
	ULONG uwp_app_count = 0;
	while (uwp_app_count < kMaxAppItemCount) {
		// Retrieve the item.
		CoPtr<IShellItem> app_item;
		if (FAILED(enum_apps->Next(1, &app_item, nullptr)) || !app_item) {
			break;
		}
		
		// Retrieve the properties of the app.
		CoPtr<IPropertyStore> app_props;
		if (FAILED(app_item->BindToHandler(nullptr, BHID_PropertyStore, IID_PPV_ARGS(&app_props)))) {
			continue;
		}
		
		// Check that the non_uwp_prop_key property value is empty, indicating an UWP app.
		PROPVARIANT non_uwp_prop_value;
		if (SUCCEEDED(app_props->GetValue(non_uwp_prop_key, &non_uwp_prop_value)) &&
				non_uwp_prop_value.vt != VT_EMPTY) {
			continue;
		}
		
		// UWP app found.
		UwpApp& uwp_app = uwp_apps[uwp_app_count++];
		uwp_app.item = std::move(app_item);
		uwp_app.props = std::move(app_props);
	}
	
	// Sort the apps by display name.
	std::sort(
		uwp_apps, uwp_apps + uwp_app_count,
		[](const UwpApp& app1, const UwpApp& app2) {
			int order = 0;
			app1.item.get()->Compare(app2.item.get(), SICHINT_DISPLAY, &order);  // ignore errors
			return order < 0;
		});
	
	// Pass the apps to the callback.
	for (ULONG i = 0; i < uwp_app_count; i++) {
		UwpApp& uwp_app = uwp_apps[i];
		
		// Retrieve the name of the app.
		String output;
		CoBuffer<LPTSTR> app_name;
		if (FAILED(uwp_app.item->GetDisplayName(SIGDN_NORMALDISPLAY, &app_name))) {
			continue;
		}
		
		// Retrieve the ID of the app.
		TCHAR app_id[kMaxAppIdLength];
		PROPVARIANT id_prop_value;
		if (FAILED(uwp_app.props->GetValue(id_prop_key, &id_prop_value)) ||
				FAILED(PropVariantToString(id_prop_value, app_id, arrayLength(app_id)))) {
			continue;
		}
		
		// Retrieve the PIDL of the app.
		CoBuffer<PIDLIST_ABSOLUTE> app_pidl;
		if (FAILED(SHGetIDListFromObject(uwp_app.item.get(), &app_pidl))) {
			continue;
		}
		
		app_callback(app_name, app_id, app_pidl);
	}
}



//------------------------------------------------------------------------
// Strings
//------------------------------------------------------------------------

LPCTSTR getSemiColonToken(LPTSTR* token_start) {
	const LPTSTR token_start_copy = *token_start;
	LPTSTR current = token_start_copy;
	while (*current != _T(';')) {
		if (!*current) {
			*token_start = current;
			return token_start_copy;
		}
		current++;
	}
	*current = _T('\0');
	*token_start = current + 1;
	return token_start_copy;
}


//------------------------------------------------------------------------
// Command line parsing and executing
//------------------------------------------------------------------------

void findFullPath(LPTSTR path, LPTSTR full_path) {
	if (!pathIsSlow(path)) {
		PathUnquoteSpaces(path);
		if (SearchPath(/* lpPath= */ nullptr, path, /* lpExtension= */ nullptr,
				MAX_PATH, full_path, /* lpFilePath= */ nullptr)) {
			return;
		}
		
		DWORD buf = MAX_PATH;
		if (SUCCEEDED(AssocQueryString(ASSOCF_OPEN_BYEXENAME, ASSOCSTR_EXECUTABLE,
				path, _T("open"), full_path, &buf))) {
			return;
		}
		
		if (32 < reinterpret_cast<UINT_PTR>(FindExecutable(path, /* lpDirectory= */ nullptr, full_path))) {
			return;
		}
	}
	
	lstrcpy(full_path, path);
}


void shellExecuteCmdLine(LPCTSTR command, LPCTSTR directory, int show_mode, bool run_as_admin) {
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
	sei.hwnd = e_invisible_window;
	sei.lpFile = path;
	sei.lpVerb = run_as_admin ? _T("runas") : nullptr;
	sei.lpParameters = PathGetArgs(command_exp);
	sei.lpDirectory = real_directory;
	sei.nShow = show_mode;
	ShellExecuteEx(&sei);
}


DWORD WINAPI ShellExecuteThread::thread(void* params) {
	ShellExecuteThread* const params_ptr = reinterpret_cast<ShellExecuteThread*>(params);
	shellExecuteCmdLine(params_ptr->m_command, params_ptr->m_directory, params_ptr->m_show_mode, params_ptr->m_run_as_admin);
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

static constexpr TCHAR autostart_key[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
static constexpr LPCTSTR autostart_value = pszApp;

static HKEY openAutoStartKey(LPTSTR autostart_path_buf);

// Open the Registry key for the auto-start setting.
//
// Also stores in autostart_path_buf the string to store under the key for the value named
// autostart_value to enable auto-start, that is the path of the Clavier+ executable.
// Expected to have a capacity >= MAX_PATH, set to empty string on error.
//
// Returns the opened key, NULL on error.
// The caller is responsible for closing the key with RegCloseKey().
HKEY openAutoStartKey(LPTSTR autostart_path_buf) {
	if (!GetModuleFileName(/* hInstance= */ NULL, autostart_path_buf, MAX_PATH)) {
		*autostart_path_buf = _T('\0');
	}
	HKEY key;
	if (ERROR_SUCCESS != RegOpenKey(HKEY_CURRENT_USER, autostart_key, &key)) {
		key = NULL;
	}
	return key;
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
	
	if (ERROR_SUCCESS == RegQueryValueEx(
				autostart_hKey, autostart_value, /* lpReserved= */ nullptr, &type,
				/* lpData= */ nullptr, &size)
			&& type == REG_SZ
			&& size < sizeof(current_path)) {
		if (ERROR_SUCCESS == RegQueryValueEx(
					autostart_hKey, autostart_value, /* lpReserved= */ nullptr, /* lpType= */ nullptr,
					/* lpData= */ reinterpret_cast<BYTE*>(current_path), &size) &&
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
		RegSetValueEx(
			autostart_hKey, autostart_value, /* Reserved= */ 0, REG_SZ,
			reinterpret_cast<const BYTE*>(autostart_path_buf),
			(lstrlen(autostart_path_buf) + 1) * sizeof(*autostart_path_buf));
	} else {
		RegDeleteValue(autostart_hKey, autostart_value);
	}
	RegCloseKey(autostart_hKey);
}
