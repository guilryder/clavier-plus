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

#ifdef __GNUC__
// Missing propsys.h declarations for MinGW.

PSSTDAPI PropVariantToString(REFPROPVARIANT propvar, PWSTR psz, UINT cch);

__CRT_UUID_DECL(IUniformResourceLocatorW, 0xCABB0DA0L, 0xDA57, 0x11CF, 0x99, 0x74, 0x00, 0x20, 0xAF, 0xD7, 0x97, 0x62);
#endif  // __GNUC__

const int kWindowClassBufferLength = 200;

HANDLE e_heap;
HINSTANCE e_instance;
HWND e_invisible_window;
HWND e_modal_dialog;
bool e_icon_visible = true;

int e_column_widths[kColCount];

SIZE e_main_dialog_size = { 0, 0 };
bool e_maximize_main_dialog = false;

TCHAR e_ini_filepath[MAX_PATH];


int messageBox(HWND hwnd, UINT string_id, UINT type, LPCTSTR arg) {
	TCHAR format[256], text[1024];
	i18n::loadStringAuto(string_id, format);
	wsprintf(text, format, arg);
	return MessageBox(hwnd, text, kAppName, type);
}


void centerParent(HWND hwnd) {
	RECT parent_rect, child_rect;
	
	const HWND hwnd_parent = GetParent(hwnd);
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	if (hwnd_parent) {
		const HMONITOR monitor = MonitorFromWindow(hwnd_parent, MONITOR_DEFAULTTONULL);
		VERIFV(monitor);
		GetMonitorInfo(monitor, &mi);
		GetWindowRect(hwnd_parent, &parent_rect);
		IntersectRect(&parent_rect, &parent_rect, &mi.rcWork);
	} else {
		const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		VERIFV(monitor);
		GetMonitorInfo(monitor, &mi);
		parent_rect = mi.rcWork;
	}
	
	GetWindowRect(hwnd, &child_rect);
	SetWindowPos(
		hwnd, /* hWndInsertAfter= */ NULL,
		(parent_rect.left + parent_rect.right + child_rect.left - child_rect.right) / 2,
		(parent_rect.top + parent_rect.bottom + child_rect.top - child_rect.bottom) / 2,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}


void getDlgItemText(HWND hdlg, UINT id, String* output) {
	const HWND hwnd = GetDlgItem(hdlg, id);
	const int buf = GetWindowTextLength(hwnd) + 1;
	GetWindowText(hwnd, output->getBuffer(buf), buf);
}


namespace {

// Window procedure for dialog box controls displaying an URL.
// The link itself is stored in ref_data.
LRESULT CALLBACK prcWebLink(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR UNUSED(subclass_id), DWORD_PTR ref_data) {
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
				/* lpFile= */ reinterpret_cast<LPCTSTR>(ref_data),
				/* lpParameters= */ nullptr,
				/* lpDirectory= */ nullptr,
				SW_SHOWDEFAULT);
			return 0;
	}
	
	return DefSubclassProc(hwnd, message, wParam, lParam);
}

}  // namespace

void initializeWebLink(HWND hdlg, UINT control_id, LPCTSTR link) {
	const HWND hwnd = GetDlgItem(hdlg, control_id);
	subclassWindow(hwnd, prcWebLink, /* ref_data= */ reinterpret_cast<DWORD_PTR>(link));
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


namespace {

// Indicates if a file is located in a slow drive (network, removable, etc.).
bool isPathSlow(LPCTSTR path) {
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

}  // namespace

bool getFileInfo(LPCTSTR path, DWORD file_attributes, SHFILEINFO& shfi, UINT flags) {
	TCHAR transformed_path[MAX_PATH];
	
	if (isPathSlow(path)) {
		// If UNC server or share, remove any trailing backslash
		// If normal file, strip the path: extension-based icon
		
		flags |= SHGFI_USEFILEATTRIBUTES;
		StringCchCopy(transformed_path, arrayLength(transformed_path), path);
		PathRemoveBackslash(transformed_path);
		const LPCTSTR path_name = PathFindFileName(path);
		if (PathIsUNCServer(transformed_path) || PathIsUNCServerShare(transformed_path)) {
			path = transformed_path;
		} else if (path_name) {
			StringCchCopy(transformed_path, arrayLength(transformed_path), path_name);
			path = transformed_path;
		}
	}
	
	for (;;) {
		const bool ok = toBool(SHGetFileInfo(path, file_attributes, &shfi, sizeof(shfi), flags));
		if (ok || (flags & SHGFI_USEFILEATTRIBUTES)) {
			return ok;
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
				if (lstrlen(clipboard_text) < kClipboardStringBugSize) {
					ok = true;
					SetEnvironmentVariable(kClipboardEnvVariableName, clipboard_text);
				}
				GlobalUnlock(clipboard_mem);
			}
		}
		CloseClipboard();
	}
	
	if (!ok) {
		SetEnvironmentVariable(kClipboardEnvVariableName, _T(""));
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


namespace {

struct FIND_WINDOW_BY_NAME {
	LPCTSTR title_regexp;
	HWND hwnd_found;
};

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

}  // namespace

HWND findWindowByName(LPCTSTR title_regexp) {
	FIND_WINDOW_BY_NAME fwbn;
	fwbn.title_regexp = title_regexp;
	fwbn.hwnd_found = NULL;
	EnumWindows(prcEnumFindWindowByName, reinterpret_cast<LPARAM>(&fwbn));
	return fwbn.hwnd_found;
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
	const int text_buf_size = lstrlen(text) + 1;
	const HGLOBAL buffer = GlobalAlloc(GMEM_MOVEABLE, text_buf_size * sizeof(*text));
	const LPTSTR locked_buffer = reinterpret_cast<LPTSTR>(GlobalLock(buffer));
	StringCchCopy(locked_buffer, text_buf_size, text);
	GlobalUnlock(buffer);
	
	// Pass the buffer to the clipboard.
	SetClipboardData(CF_UNICODETEXT, buffer);
	CloseClipboard();
}


namespace {

int CALLBACK prcBrowseForFolderCallback(HWND hwnd, UINT message, LPARAM UNUSED(lParam), LPARAM lpData) {
	if (message == BFFM_INITIALIZED) {
		SendMessage(hwnd, BFFM_SETSELECTION, true, lpData);
	}
	return 0;
}

}  // namespace

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


//------------------------------------------------------------------------
// Shell API tools
//------------------------------------------------------------------------

bool getSpecialFolderPath(int csidl, LPTSTR path) {
	return toBool(SHGetSpecialFolderPath(/* hWnd= */ NULL, path, csidl, /* fCreate= */ true));
}


void resolveLinkFile(LPCTSTR link_file, Shortcut* shortcut) {
	shortcut->m_type = Shortcut::Type::kCommand;
	String& cmdline = shortcut->m_command;
	
	TCHAR product_code[39];
	TCHAR component_code[39];
	if (!MsiGetShortcutTarget(link_file, product_code, /* szFeatureId= */ nullptr, component_code)) {
		DWORD buf_size = MAX_PATH;
		MsiGetComponentPath(product_code, component_code, cmdline.getBuffer(buf_size), &buf_size);
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
					SUCCEEDED(url_locator->GetURL(url.outPtr())) &&
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
	VERIFV(SUCCEEDED(manager->GetFolder(FOLDERID_AppsFolder, known_folder.outPtr())) &&
		SUCCEEDED(known_folder->GetShellItem(KF_FLAG_DEFAULT, IID_PPV_ARGS(folder_item.outPtr()))));
	
	// Property set only on non-UWP apps.
	PROPERTYKEY non_uwp_prop_key;
	VERIFV(SUCCEEDED(PSGetPropertyKeyFromName(_T("System.Link.TargetParsingPath"), &non_uwp_prop_key)));
	
	// Contains the app ID for UWP apps.
	PROPERTYKEY id_prop_key;
	VERIFV(SUCCEEDED(PSGetPropertyKeyFromName(_T("System.AppUserModel.ID"), &id_prop_key)));
	
	// Enumerate the apps folder children, UWP and non-UWP apps alike.
	CoPtr<IEnumShellItems> enum_apps;
	VERIFV(SUCCEEDED(folder_item->BindToHandler(nullptr, BHID_StorageEnum, IID_PPV_ARGS(enum_apps.outPtr()))));
	
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
		if (FAILED(enum_apps->Next(1, app_item.outPtr(), nullptr)) || !app_item) {
			break;
		}
		
		// Retrieve the properties of the app.
		CoPtr<IPropertyStore> app_props;
		if (FAILED(app_item->BindToHandler(nullptr, BHID_PropertyStore, IID_PPV_ARGS(app_props.outPtr())))) {
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
		if (FAILED(uwp_app.item->GetDisplayName(SIGDN_NORMALDISPLAY, app_name.outPtr()))) {
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
		if (FAILED(SHGetIDListFromObject(uwp_app.item.get(), app_pidl.outPtr()))) {
			continue;
		}
		
		app_callback(app_name, app_id, app_pidl);
	}
}


//------------------------------------------------------------------------
// Strings
//------------------------------------------------------------------------

void unescape(LPTSTR str) {
	const TCHAR* input = str;
	TCHAR* output = str;
	bool escaping = false;
	while (*input) {
		if (*input == _T('\\') && !escaping) {
			escaping = true;
		} else {
			escaping = false;
			*output++ = *input;
		}
		input++;
	}
	*output = _T('\0');
}


LPCTSTR parseCommaSepArg(TCHAR*& chr_ptr, bool unescape) {
	const LPTSTR start = chr_ptr;
	bool escaping = false;
	TCHAR* current = start;
	while (*current && !(*current == _T(',') && !escaping)) {
		escaping = !escaping && (unescape && *current == _T('\\'));
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
		::unescape(start);
	}
	
	return start;
}


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
	if (!isPathSlow(path)) {
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
			return;  // Successful.
		}
	}
	
	StringCchCopy(full_path, MAX_PATH, path);
}


void shellExecuteCmdLine(LPCTSTR command, LPCTSTR directory, int show_mode) {
	// Expand the environment variables before splitting
	TCHAR command_exp[MAX_PATH + kClipboardStringBugSize];
	ExpandEnvironmentStrings(command, command_exp, arrayLength(command_exp));
	
	// Split the command line: get the file and the arguments
	TCHAR path[MAX_PATH];
	StringCchCopy(path, arrayLength(path), command_exp);
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
	sei.lpVerb = nullptr;
	sei.lpParameters = PathGetArgs(command_exp);
	sei.lpDirectory = real_directory;
	sei.nShow = show_mode;
	ShellExecuteEx(&sei);
}


DWORD WINAPI ShellExecuteThread::thread(void* params) {
	ShellExecuteThread *const params_ptr = reinterpret_cast<ShellExecuteThread*>(params);
	shellExecuteCmdLine(params_ptr->m_command, params_ptr->m_directory, params_ptr->m_show_mode);
	delete params_ptr;
	return 0;
}


//------------------------------------------------------------------------
// Autostart setting
// Persisted in the Registry under the key autostart_key, in a value
// named autostart_value.
//------------------------------------------------------------------------

namespace {

constexpr LPCTSTR kAutostartRegKey = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
constexpr LPCTSTR kAutostartRegValue = kAppName;

// Open the Registry key for the auto-start setting.
//
// Also stores in cmdline_buf the string to store under the key for the value named
// kAutostartRegValue to enable auto-start: the path of the Clavier+ executable,
// quoted and with the /launch argument. Must be kept in such 
// Expected to have a capacity >= MAX_PATH, set to empty string on error.
//
// Returns the opened key, NULL on error.
// The caller is responsible for closing the key with RegCloseKey().
HKEY openAutoStartKey(LPTSTR cmdline_buf) {
	// Format the command-line the same way as the installation script:
	// no PathQuoteSpaces().
	if (GetModuleFileName(/* hInstance= */ NULL, cmdline_buf + 1, MAX_PATH - 1)) {
		cmdline_buf[0] = _T('"');
		StringCchCat(cmdline_buf, MAX_PATH, _T("\" /launch"));
	} else {
		*cmdline_buf = _T('\0');
	}
	HKEY key;
	if (ERROR_SUCCESS != RegOpenKey(HKEY_CURRENT_USER, kAutostartRegKey, &key)) {
		key = NULL;
	}
	return key;
}

}  // namespace

bool isAutoStartEnabled() {
	TCHAR cmdline_buf[MAX_PATH];
	const HKEY hkey = openAutoStartKey(cmdline_buf);
	if (!hkey) {
		return false;
	}
	
	DWORD type, size;
	TCHAR cmdline_current[MAX_PATH];
	
	bool enabled = false;
	if (ERROR_SUCCESS == RegQueryValueEx(
				hkey, kAutostartRegValue, /* lpReserved= */ nullptr, &type,
				/* lpData= */ nullptr, &size)
			&& type == REG_SZ
			&& size < sizeof(cmdline_current)) {
		if (ERROR_SUCCESS == RegQueryValueEx(
					hkey, kAutostartRegValue, /* lpReserved= */ nullptr, /* lpType= */ nullptr,
					/* lpData= */ reinterpret_cast<BYTE*>(cmdline_current), &size) &&
				!lstrcmpi(cmdline_current, cmdline_buf)) {
			enabled = true;
		}
	}
	
	RegCloseKey(hkey);
	return enabled;
}

void setAutoStartEnabled(bool enabled) {
	TCHAR cmdline_buf[MAX_PATH];
	const HKEY hkey = openAutoStartKey(cmdline_buf);
	if (!hkey) {
		return;
	}
	
	if (enabled) {
		RegSetValueEx(
			hkey, kAutostartRegValue, /* Reserved= */ 0, REG_SZ,
			reinterpret_cast<const BYTE*>(cmdline_buf),
			(lstrlen(cmdline_buf) + 1) * sizeof(*cmdline_buf));
	} else {
		RegDeleteValue(hkey, kAutostartRegValue);
	}
	RegCloseKey(hkey);
}
