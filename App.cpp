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
#include "App.h"
#include "Dialogs.h"
#include "Shortcut.h"

#ifdef _DEBUG
// #define ALLOW_MULTIPLE_INSTANCES
#endif


namespace app {
namespace {

constexpr LPCTSTR kClavierWindowClass = _T("RyderClavierWindow");

UINT s_taskbar_created_message;
UINT s_notify_icon_message;

constexpr int kMaxIniFile = 20;

TranslatedString s_tokens[int(Token::kNotFound)];


enum class CmdlineOpt {
	// No arguments.
	kLaunch,
	kSettings,
	kMenu,
	kQuit,
	kAddText,
	kAddCommand,
	
	// Some arguments.
	kWithArg,
	kLoad = kWithArg,
	kMerge,
	kSendKeys,
	
	kNone
};

// Name of each CmdlineOpt except kNone.
constexpr LPCTSTR kCmdlineOptions[] = {
	_T("launch"),
	_T("settings"),
	_T("menu"),
	_T("quit"),
	_T("addtext"),
	_T("addcommand"),
	_T("load"),
	_T("merge"),
	_T("sendkeys"),
};

void entryPoint();
void initializeLanguages();
void runGui(CmdlineOpt cmdopt);
CmdlineOpt execCmdLine(LPCTSTR cmdline, bool initial_launch);
void processCmdLineAction(CmdlineOpt cmdopt);

LRESULT CALLBACK prcInvisible(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR subclass_id, DWORD_PTR ref_data);

void updateTrayIcon(DWORD message);

// Displays the tray icon menu, as a child of the invisible window.
//
// Returns:
//   The ID of the chosen command, 0 if the user cancelled.
UINT displayTrayIconMenu();

}  // namespace
}  // namespace app


#ifdef _DEBUG

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	app::entryPoint();
	return 0;
}

#else

extern "C" void WinMainCRTStartup() {
	app::entryPoint();
}

extern "C" void abort() {
	ExitProcess(3);
}

extern "C" int _purecall() {
	abort();
}

#endif  // _DEBUG


namespace app {
namespace {

void entryPoint() {
	const LPCTSTR cmdline = GetCommandLine();
	
#ifndef ALLOW_MULTIPLE_INSTANCES
	// Avoid multiple launching
	// Transmit the command line to the previous instance, if any
	{
		const HWND hwnd = FindWindow(_T("STATIC"), kClavierWindowClass);
		if (hwnd) {
			COPYDATASTRUCT cds = {
				.dwData = true,
				.cbData = static_cast<DWORD>((lstrlen(cmdline) + 1) * sizeof(*cmdline)),
				.lpData = const_cast<LPTSTR>(cmdline),
			};
			SendMessage(hwnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds));
			
#ifdef _DEBUG
			return;
#else
			ExitProcess(0);
#endif  // _DEBUG
		}
	}
#endif  // ALLOW_MULTIPLE_INSTANCES
	
	e_instance = HINSTANCE(GetModuleHandle(/* lpModuleName= */ nullptr));
	app::initialize();
	
	const CmdlineOpt cmdopt = execCmdLine(cmdline, /* initial_launch= */ true);
	if (cmdopt != CmdlineOpt::kQuit) {
		runGui(cmdopt);
	}
	
	app::terminate();
#ifndef _DEBUG
	ExitProcess(0);
#endif  // _DEBUG
}

}  // namespace

void initialize() {
	s_taskbar_created_message = RegisterWindowMessage(_T("TaskbarCreated"));
	s_notify_icon_message = RegisterWindowMessage(_T("RyderClavierOptions"));
	
	CoInitialize(/* pvReserved= */ nullptr);
	shortcut::initialize();
	
	e_heap = GetProcessHeap();
	e_modal_dialog = NULL;
	
	initializeLanguages();
	Keystroke::loadVkKeyNames();
}

void terminate() {
	shortcut::terminate();
	CoUninitialize();
}

namespace {

void initializeLanguages() {
	TCHAR tokens[512];
	for (int lang = 0; lang < i18n::kLangCount; lang++) {
		i18n::setLanguage(i18n::Language(lang));
		
		// Load all tokens in the current language.
		i18n::loadStringAuto(IDS_TOKENS, tokens);
		TCHAR* next_token = tokens;
		for (int tok = 0; *next_token; tok++) {
			s_tokens[tok].set(getSemiColonToken(&next_token));
		}
		
		dialogs::initializeCurrentLanguage();
	}
	
	i18n::setLanguage(i18n::getDefaultLanguage());
}

void runGui(CmdlineOpt cmdopt) {
	// Create the invisible window
	e_invisible_window = CreateWindow(
		_T("STATIC"), kClavierWindowClass,
		/* dwStyle=*/ 0,
		/* x,y,nWidth,nHeight=*/ 0,0,0,0,
		/* hWndParent= */ NULL, /* hMenu= */ NULL, e_instance, /* lpParam= */ nullptr);
	subclassWindow(e_invisible_window, prcInvisible);
	
	// Create the traybar icon
	updateTrayIcon(NIM_ADD);
	
	processCmdLineAction(cmdopt);
	
	// Message loop
	DWORD timeMinimum = 0;
	DWORD timeLast = GetTickCount();
	MSG msg;
	while (GetMessage(&msg, /* hWnd= */ NULL, 0, 0)) {
		if (msg.message == WM_HOTKEY) {
			// Shortcut
			
			if (msg.time < timeLast) {
				timeMinimum = 0;
			}
			if (msg.time < timeMinimum) {
				goto Next;
			}
			
			Keystroke ks;
			ks.m_vk = Keystroke::canonicalizeKey(BYTE(HIWORD(msg.lParam)));
			ks.m_sided_mod_code = LOWORD(msg.lParam);
			ks.m_sided = true;
			const HWND input_window = Keystroke::getInputFocus();
			
			// Test for right special keys
			for (const auto& special_key : kSpecialKeys) {
				const DWORD mod_code = special_key.mod_code;
				if (ks.m_sided_mod_code & mod_code) {
					if (isKeyDown(special_key.vk_right)) {
						ks.m_sided_mod_code &= ~mod_code;
						ks.m_sided_mod_code |= mod_code << kRightModCodeOffset;
					}
				}
			}
			
			// Get the toggle keys state, for conditions checking
			for (int i = 0; i < Keystroke::kCondTypeCount; i++) {
				ks.m_conditions[i] = (GetKeyState(Keystroke::kCondTypeVks[i]) & kKeyToggledMask)
					? Keystroke::Condition::kYes
					: Keystroke::Condition::kNo;
			}
			
			// Get the current program, for conditions checking
			TCHAR process[MAX_PATH];
			if (!getWindowProcessName(input_window, process)) {
				*process = _T('\0');
			}
			
			// Find and execute the shortcut.
			Shortcut *const sh = shortcut::find(ks, (*process) ? process : nullptr);
			if (sh) {
				sh->execute(/* from_hotkey= */ true);
				if (sh->m_type == Shortcut::Type::kText) {
					timeMinimum = GetTickCount();
				}
			} else {
				// No matching shortcut found: simulate the keystroke back for default processing.
				// Do not press the special keys again, do not release them.
				ks.simulateTyping(/* already_down_mod_code= */ ks.m_sided_mod_code);
			}
			
		} else {
			// Other message
			
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		
	Next:
		timeLast = msg.time;
	}
	
	// Delete traybar icon
	updateTrayIcon(NIM_DELETE);
	
	// Delete the shortcuts. No saving necessary.
	shortcut::clearShortcuts();
}



// Returns the action to execute.
// initial_launch: true for regular execution, false if the command line is sent by WM_COPYDATA.
CmdlineOpt execCmdLine(LPCTSTR cmdline, bool initial_launch) {
	bool new_ini_file = false;  // True if loading a new INI file is asked
	
	bool in_quote;
	const TCHAR* pstr = cmdline;
	TCHAR chr;
	
	// Skip program name
	in_quote = false;
	do {
		chr = *pstr++;
		if (chr == _T('"')) {
			in_quote ^= true;
		}
	} while (chr && (in_quote || (chr != _T(' ') && chr != _T('\t'))));
	
	in_quote = false;
	
	CmdlineOpt action_cmdopt = CmdlineOpt::kNone;
	bool can_auto_quit = true;
	bool try_auto_quit = false;
	bool default_action = true;
	String mergeable_ini_files[kMaxIniFile];
	int mergeable_ini_files_count = 0;
	
	int args_count = 0;
	if (chr) {
		// Parse arguments
		
		CmdlineOpt cmdopt = CmdlineOpt::kNone;
		
		const LPTSTR strbuf_arg = new TCHAR[lstrlen(cmdline) + 1];
		
		for (;;) {
			
			// Skip argument separator
			while (*pstr == _T(' ') || *pstr == '\t') {
				pstr++;
			}
			if (!*pstr) {
				break;
			}
			
			args_count++;
			TCHAR* str_arg = strbuf_arg;
			for (;;) {
				bool copy_char = true;  // True to copy the character to strbuf_arg
				
				// Count backslashes
				int backslash_count = 0;
				while (*pstr == _T('\\')) {
					pstr++;
					backslash_count++;
				}
				
				// Process quoting
				if (*pstr == _T('"')) {
					if (backslash_count % 2 == 0) {
						if (in_quote && pstr[1] == _T('"')) {
							pstr++;
						} else {
							copy_char = false;
							in_quote ^= true;
						}
					}
					backslash_count /= 2;
				}
				
				while (backslash_count--) {
					*str_arg++ = _T('\\');
				}
				
				// Stop on spaces
				chr = *pstr;
				if (!chr || (!in_quote && (chr == _T(' ') || chr == _T('\t')))) {
					break;
				}
				
				if (copy_char) {
					*str_arg++ = chr;
				}
				pstr++;
			}
			*str_arg = _T('\0');
			
			if (cmdopt == CmdlineOpt::kNone) {
				// Test for command line option
				if (*strbuf_arg == _T('/')) {
					const LPCTSTR options = strbuf_arg + 1;
					for (cmdopt = CmdlineOpt(0); cmdopt < CmdlineOpt::kNone; cmdopt = CmdlineOpt(int(cmdopt) + 1)) {
						if (!lstrcmpi(options, kCmdlineOptions[int(cmdopt)])) {
							break;
						}
					}
				}
				
				if (cmdopt >= CmdlineOpt::kWithArg) {
					continue;
				}
			}
			
			default_action = false;
			switch (cmdopt) {
				
				// Set INI filename
				case CmdlineOpt::kLoad:
					can_auto_quit = false;
					new_ini_file = true;
					GetFullPathName(
						strbuf_arg,
						arrayLength(e_ini_filepath), e_ini_filepath,
						/* lpFilePart= */ nullptr);
					break;
				
				// Merge an INI file
				case CmdlineOpt::kMerge:
					can_auto_quit = false;
					if (mergeable_ini_files_count < arrayLength(mergeable_ini_files)) {
						mergeable_ini_files[mergeable_ini_files_count++] = strbuf_arg;
					}
					break;
				
				// Send keys
				case CmdlineOpt::kSendKeys: {
					try_auto_quit = true;
					Shortcut shortcut;
					shortcut.m_type = Shortcut::Type::kText;
					shortcut.m_text = strbuf_arg;
					shortcut.execute(/* from_hotkey= */ false);
					break;
				}
				
				// Other action
				default:
					action_cmdopt = cmdopt;
					can_auto_quit = false;
					break;
			}
			
			cmdopt = CmdlineOpt::kNone;
		}
		
		delete [] strbuf_arg;
	}
	
	bool auto_quit = can_auto_quit && try_auto_quit;
	
	// Default INI file
	if (initial_launch && !new_ini_file && !auto_quit) {
		new_ini_file = true;
		GetModuleFileName(e_instance, e_ini_filepath, arrayLength(e_ini_filepath));
		PathRemoveFileSpec(e_ini_filepath);
		PathAppend(e_ini_filepath, _T("Clavier.ini"));
	}
	
	if (!e_modal_dialog) {
		if (new_ini_file) {
			shortcut::loadShortcuts();
		}
		
		for (int ini_file = 0; ini_file < mergeable_ini_files_count; ini_file++) {
			shortcut::mergeShortcuts(mergeable_ini_files[ini_file]);
		}
		
		if (mergeable_ini_files_count > 0) {
			shortcut::saveShortcuts();
		}
	}
	
	if (action_cmdopt == CmdlineOpt::kNone) {
		if (initial_launch) {
			action_cmdopt = auto_quit ? CmdlineOpt::kQuit : CmdlineOpt::kLaunch;
		} else {
			action_cmdopt = default_action ? CmdlineOpt::kSettings : CmdlineOpt::kLaunch;
		}
	}
	
	return action_cmdopt;
}


void processCmdLineAction(CmdlineOpt cmdopt) {
	LPARAM lParam_to_send;
	WPARAM wParam_to_send;
	switch (cmdopt) {
		
		case CmdlineOpt::kSettings:
			lParam_to_send = WM_LBUTTONUP;
			wParam_to_send = 0;
			break;
		
		case CmdlineOpt::kMenu:
			lParam_to_send = WM_RBUTTONUP;
			wParam_to_send = 0;
			break;
		
		case CmdlineOpt::kQuit:
			lParam_to_send = WM_COMMAND;
			wParam_to_send = IDCCMD_QUIT;
			break;
		
		case CmdlineOpt::kAddText:
			lParam_to_send = WM_COMMAND;
			wParam_to_send = ID_ADD_TEXT;
			break;
		
		case CmdlineOpt::kAddCommand:
			lParam_to_send = WM_COMMAND;
			wParam_to_send = ID_ADD_COMMAND;
			break;
		
		default:
			return;
	}
	PostMessage(e_invisible_window, s_notify_icon_message, wParam_to_send, lParam_to_send);
}



// Invisible window:
// - quit when destroyed
// - handle traybar icon notifications
LRESULT CALLBACK prcInvisible(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR UNUSED(subclass_id), DWORD_PTR UNUSED(ref_data)) {
	if (message == WM_DESTROY) {
Destroy:
		PostQuitMessage(0);
		
	} else if (message == s_notify_icon_message) {
		if (lParam == WM_COMMAND && wParam == IDCCMD_QUIT) {
			goto Destroy;
		}
		
		// WM_LBUTTONUP is a special case of WM_COMMAND:
		// show the configuration dialog then execute no command.
		if (lParam == WM_LBUTTONUP) {
			lParam = WM_COMMAND;
			wParam = 0;
		}
		
		if (lParam != WM_COMMAND && lParam != WM_RBUTTONUP) {
			return 0;
		}
		
		if (e_modal_dialog) {
			// A modal dialog is visible: give it the focus.
			// Execute the WM_COMMAND (if any), but only
			// if just the configuration dialog is visible, not a child dialog,
			// to avoid opening another child dialog.
			SetForegroundWindow(e_modal_dialog);
			if (e_modal_dialog == dialogs::e_hdlgMain && lParam == WM_COMMAND && wParam != 0) {
				PostMessage(dialogs::e_hdlgMain, WM_COMMAND, wParam, 0);
			}
			return 0;
		}
		
		if (lParam == WM_COMMAND) {
			// Left click or command: display the configuration dialog.
			
			for (;;) {
				HeapCompact(e_heap, 0);
				switch (dialogs::showMainDialogModal(UINT(wParam))) {
					case IDCCMD_LANGUAGE:
						break;
					case IDCCMD_QUIT:
						goto Destroy;
					default:
						HeapCompact(e_heap, 0);
						return 0;
				}
			}
			
		} else /* (lParam == WM_RBUTTONUP)*/ {
			// Right click: display the tray menu
			
			if (displayTrayIconMenu() == ID_TRAY_QUIT) {
				goto Destroy;
			}
		}
		
	} else if (message == s_taskbar_created_message) {
		updateTrayIcon(NIM_ADD);
	
	} else if (message == WM_COPYDATA) {
		// Execute command line
		
		const auto& cds = *reinterpret_cast<const COPYDATASTRUCT*>(lParam);
		if (cds.dwData) {
			processCmdLineAction(execCmdLine(LPCTSTR(cds.lpData), /* initial_launch= */ false));
		}
		return true;
	}
	
	return DefSubclassProc(hwnd, message, wParam, lParam);
}



// Handle the tray icon
void updateTrayIcon(DWORD message) {
	VERIFV(e_icon_visible);
	
	NOTIFYICONDATA nid;
	ZeroMemory(&nid, sizeof(nid));
	nid.cbSize = sizeof(nid);
	nid.hWnd = e_invisible_window;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = s_notify_icon_message;
	if (message != NIM_DELETE) {
		nid.hIcon = i18n::loadNeutralIcon(IDI_APP, 16,16);
		StringCchCopy(nid.szTip, arrayLength(nid.szTip), kAppName);
	}
	Shell_NotifyIcon(message, &nid);
}


UINT displayTrayIconMenu() {
	const HWND hwnd = e_invisible_window;
	
	const HMENU all_menus = i18n::loadMenu(IDM_CONTEXT);
	const HMENU menu = GetSubMenu(all_menus, 2);
	SetMenuDefaultItem(menu, ID_TRAY_SETTINGS, MF_BYCOMMAND);
	
	// Append INI file items
	
	// 1) Add current INI file
	LPTSTR ini_files[kMaxIniFile];
	ini_files[0] = e_ini_filepath;
	UINT ini_files_count = 1;
	
	// 2) Ad INI files from Clavier+ directory
	TCHAR ini_files_spec[MAX_PATH];
	GetModuleFileName(e_instance, ini_files_spec, arrayLength(ini_files_spec));
	PathRemoveFileSpec(ini_files_spec);
	PathAppend(ini_files_spec, _T("*.ini"));
	WIN32_FIND_DATA wfd;
	const HANDLE hff = FindFirstFile(ini_files_spec, &wfd);
	PathRemoveFileSpec(ini_files_spec);
	if (hff != INVALID_HANDLE_VALUE) {
		do {
			if (!(wfd.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN))) {
				const LPTSTR path = new TCHAR[MAX_PATH];
				PathCombine(path, ini_files_spec, wfd.cFileName);
				if (lstrcmpi(path, e_ini_filepath)) {
					ini_files[ini_files_count++] = path;
					if (ini_files_count == kMaxIniFile) {
						break;
					}
				} else {
					delete [] path;
				}
			}
		} while (FindNextFile(hff, &wfd));
		FindClose(hff);
	}
	
	// 2) Append INI files to menu
	for (UINT ini_file = 0; ini_file < ini_files_count; ini_file++) {
		InsertMenu(menu, ini_file, MF_BYPOSITION | MF_STRING, ID_TRAY_INI_FIRSTFILE + ini_file,
				PathFindFileName(ini_files[ini_file]));
	}
	CheckMenuRadioItem(menu, ID_TRAY_INI_FIRSTFILE, ID_TRAY_INI_FIRSTFILE + ini_files_count - 1,
		ID_TRAY_INI_FIRSTFILE, MF_BYCOMMAND);
	
	// Show the popup menu
	POINT cursor_pos;
	GetCursorPos(&cursor_pos);
	SetForegroundWindow(hwnd);
	UINT id = TrackPopupMenu(
		menu,
		TPM_RETURNCMD | TPM_NONOTIFY,
		cursor_pos.x, cursor_pos.y,
		/* nReserved= */ 0,
		hwnd,
		/* prcRect= */ nullptr);
	PostMessage(hwnd, WM_NULL, 0, 0);
	DestroyMenu(all_menus);
	
	switch (id) {
		case ID_TRAY_SETTINGS:
			PostMessage(hwnd, s_notify_icon_message, 0, WM_COMMAND);
			break;
		
		case ID_TRAY_COPYLIST: {
			String str;
			for (const Shortcut* sh = shortcut::getFirst(); sh; sh = sh->getNext()) {
				sh->appendCsvLineToString(str);
			}
			setClipboardText(str);
		}
		break;
		
		case ID_TRAY_INI_LOAD:
		case ID_TRAY_INI_MERGE:
		case ID_TRAY_INI_SAVE: {
			// Load the filters, replace '|' with '\0'
			TCHAR filters[kStringBufSize];
			i18n::loadStringAuto(IDS_INI_FILTER, filters);
			for (UINT chr_index = 0; filters[chr_index]; chr_index++) {
				if (filters[chr_index] == _T('|')) {
					filters[chr_index] = _T('\0');
				}
			}
			
			TCHAR ini_file[arrayLength(e_ini_filepath)];
			StringCchCopy(ini_file, arrayLength(ini_file), e_ini_filepath);
			
			OPENFILENAME ofn;
			ZeroMemory(&ofn, OPENFILENAME_SIZE_VERSION_400);
			ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
			ofn.hwndOwner = hwnd;
			ofn.lpstrFile = ini_file;
			ofn.nMaxFile = arrayLength(ini_file);
			ofn.lpstrFilter = filters;
			if (id == ID_TRAY_INI_SAVE) {
				ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
				if (GetSaveFileName(&ofn)) {
					StringCchCopy(e_ini_filepath, arrayLength(e_ini_filepath), ini_file);
					shortcut::saveShortcuts();
				}
			} else {
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
				if (GetOpenFileName(&ofn)) {
					if (id == ID_TRAY_INI_LOAD) {
						StringCchCopy(e_ini_filepath, arrayLength(e_ini_filepath), ini_file);
						shortcut::loadShortcuts();
					} else {
						shortcut::mergeShortcuts(ini_file);
						shortcut::saveShortcuts();
					}
				}
			}
			break;
		}
		
		default:
			if (id > ID_TRAY_INI_FIRSTFILE) {
				// INI file different than the current one
				StringCchCopy(
					e_ini_filepath, arrayLength(e_ini_filepath),
					ini_files[id - ID_TRAY_INI_FIRSTFILE]);
				shortcut::loadShortcuts();
			}
			break;
	}
	
	for (UINT ini_file = 1; ini_file < ini_files_count; ini_file++) {
		delete [] ini_files[ini_file];
	}
	
	return id;
}

}  // namespace
}  // namespace app



LPCTSTR getToken(Token tok) {
	return app::s_tokens[int(tok)].get();
}

LPCTSTR getLanguageName(i18n::Language lang) {
	return app::s_tokens[int(Token::kLanguageName)].get(lang);
}

Token findToken(LPCTSTR token) {
	for (Token tok = Token::kFirst; tok < Token::kNotFound; tok++) {
		for (int lang = 0; lang < i18n::kLangCount; lang++) {
			if (!lstrcmpi(token, app::s_tokens[int(tok)].get(lang))) {
				return tok;
			}
		}
	}
	return Token::kNotFound;
}
