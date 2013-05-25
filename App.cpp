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
#include "App.h"
#include "Dialogs.h"
#include "Hook.h"
#include "Shortcut.h"

#ifdef _DEBUG
// #define ALLOW_MULTIPLE_INSTANCES
#endif

#ifdef UNICODE
const bool is_unicode = true;
#else
const bool is_unicode = false;
#endif


namespace app {

const LPCTSTR kClavierWindowClass = _T("RyderClavierWindow");

static UINT msgTaskbarCreated;
static UINT msgClavierNotifyIcon;

const int maxIniFile = 20;

static TranslatedString s_asToken[tokNotFound];

bool e_bReadOnly;


enum CMDLINE_OPTION {
	cmdoptLaunch,
	cmdoptSettings,
	cmdoptMenu,
	cmdoptQuit,
	cmdoptAddText,
	cmdoptAddCommand,
	cmdoptReadOnly,
	
	cmdoptWithArg,
	cmdoptLoad = cmdoptWithArg,
	cmdoptMerge,
	cmdoptSendKeys,
	
	cmdoptNone
};

static void entryPoint();
static void initializeLanguages();
static void runGui(CMDLINE_OPTION cmdopt);
static CMDLINE_OPTION execCmdLine(LPCTSTR cmdline, bool normal_launch);
static void processCmdLineAction(CMDLINE_OPTION cmdopt);

static LRESULT CALLBACK prcInvisible(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void updateTrayIcon(DWORD message);

// Displays the tray icon menu, as a child of the invisible window.
//
// Returns:
//   The ID of the chosen command, 0 if the user cancelled.
static UINT displayTrayIconMenu();

}  // app namespace


#ifdef _DEBUG
static void WinMainCRTStartup();

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	app::entryPoint();
	return 0;
}

#else

void WinMainCRTStartup() {
	app::entryPoint();
}

#endif  // _DEBUG


namespace app {

void entryPoint() {
	const LPCTSTR cmdline = GetCommandLine();
	
#ifndef ALLOW_MULTIPLE_INSTANCES
	// Avoid multiple launching
	// Transmit the command line to the previous instance, if any
	{
		const HWND hwnd = FindWindow(_T("STATIC"), kClavierWindowClass);
		if (hwnd) {
			COPYDATASTRUCT cds;
			cds.dwData = is_unicode;
			cds.cbData = (lstrlen(cmdline) + 1) * sizeof(TCHAR);
			cds.lpData = const_cast<LPTSTR>(cmdline);
			SendMessage(hwnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds));
			
#ifdef _DEBUG
			return;
#else
			ExitProcess(0);
#endif  // _DEBUG
		}
	}
#endif  // ALLOW_MULTIPLE_INSTANCES
	
	e_bReadOnly = false;
	
	app::initialize();
	
	const CMDLINE_OPTION cmdopt = execCmdLine(cmdline, true);
	if (cmdopt != cmdoptQuit) {
		runGui(cmdopt);
	}
	
	app::terminate();
#ifndef _DEBUG
	ExitProcess(0);
#endif  // _DEBUG
}

void initialize() {
	msgTaskbarCreated = RegisterWindowMessage(_T("TaskbarCreated"));
	msgClavierNotifyIcon = RegisterWindowMessage(_T("RyderClavierOptions"));
	
	CoInitialize(NULL);
	shortcut::initialize();
	
	e_hInst = static_cast<HINSTANCE>(GetModuleHandle(NULL));
	e_hHeap = GetProcessHeap();
	e_hdlgModal = NULL;
	
	initializeLanguages();
}

void terminate() {
	shortcut::terminate();
	CoUninitialize();
}


void initializeLanguages() {
	TCHAR tokens[512];
	for (int lang = 0; lang < i18n::langCount; lang++) {
		i18n::setLanguage(lang);
		
		// Load all tokens in the current language.
		i18n::loadStringAuto(IDS_TOKENS, tokens);
		TCHAR* token_start = tokens;
		for (int token_index = 0; token_index < tokNotFound; token_index++) {
			TCHAR* token_end = token_start;
			while (*token_end != _T(';')) {
				token_end++;
			}
			*token_end = _T('\0');
			s_asToken[token_index].set(token_start);
			token_start = token_end + 1;
		}
		
		dialogs::initializeCurrentLanguage();
	}
	
	i18n::setLanguage(i18n::getDefaultLanguage());
}

void runGui(CMDLINE_OPTION cmdopt) {
	// Create the invisible window
	e_hwndInvisible = CreateWindow(_T("STATIC"),
		kClavierWindowClass, 0, 0,0,0,0, NULL, NULL, e_hInst, NULL);
	subclassWindow(e_hwndInvisible, prcInvisible);
	
	// Create the traybar icon
	updateTrayIcon(NIM_ADD);
	
	keyboard_hook::install();
	
	processCmdLineAction(cmdopt);
	
	// Message loop
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	keyboard_hook::uninstall();
	
	// Delete traybar icon
	updateTrayIcon(NIM_DELETE);
	
	// Save shortcuts, then delete them
	shortcut::saveShortcuts();
	shortcut::clearShortcuts();
}


// Name of the command-line options. The array is indexed by CMDLINE_OPTION.
static const LPCTSTR cmdline_options[] = {
	_T("launch"),
	_T("settings"),
	_T("menu"),
	_T("quit"),
	_T("addtext"),
	_T("addcommand"),
	_T("readonly"),
	_T("load"),
	_T("merge"),
	_T("sendkeys"),
};



// bLaunching is FALSE if the command line is sent by WM_COPYDATA.
// Return the action to execute.
CMDLINE_OPTION execCmdLine(LPCTSTR cmdline, bool normal_launch) {
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
	
	CMDLINE_OPTION cmdopt = cmdoptNone;
	bool auto_quit = false;
	LPTSTR mergeable_ini_files[maxIniFile];
	int mergeable_ini_files_count = 0;
	
	int args_count = 0;
	if (chr) {
		// Parse arguments
		
		CMDLINE_OPTION cmdopt = cmdoptNone;
		
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
			
			if (cmdopt == cmdoptNone) {
				// Test for command line option
				if (*strbuf_arg == _T('/')) {
					const LPCTSTR pszOption = strbuf_arg + 1;
					for (cmdopt = static_cast<CMDLINE_OPTION>(0); cmdopt < cmdoptNone;
							cmdopt = static_cast<CMDLINE_OPTION>(cmdopt + 1)) {
						if (!lstrcmpi(pszOption, cmdline_options[cmdopt])) {
							break;
						}
					}
				}
				
				if (cmdopt >= cmdoptWithArg) {
					continue;
				}
			}
			
			switch (cmdopt) {
				
				// Set read-only mode
				case cmdoptReadOnly:
					e_bReadOnly = true;
					break;
				
				// Set INI filename
				case cmdoptLoad:
					auto_quit = false;
					new_ini_file = true;
					GetFullPathName(strbuf_arg, arrayLength(e_pszIniFile), e_pszIniFile, NULL);
					break;
				
				// Merge an INI file
				case cmdoptMerge:
					auto_quit = false;
					if (mergeable_ini_files_count < arrayLength(mergeable_ini_files)) {
						const LPTSTR ini_file = new TCHAR[lstrlen(strbuf_arg) + 1];
						lstrcpy(ini_file, strbuf_arg);
						mergeable_ini_files[mergeable_ini_files_count++] = ini_file;
					}
					break;
				
				// Send keys
				case cmdoptSendKeys:
					auto_quit = true;
					{
						Keystroke ks;
						Shortcut sh(ks);
						sh.m_bCommand = false;
						sh.m_sText = strbuf_arg;
						sh.execute(false);
					}
					break;
				
				// Other action
				default:
					cmdopt = cmdopt;
					auto_quit = false;
					break;
			}
			
			cmdopt = cmdoptNone;
		}
		
		delete [] strbuf_arg;
	}
	
	// Default INI file
	if (normal_launch && !new_ini_file && !auto_quit) {
		new_ini_file = true;
		GetModuleFileName(e_hInst, e_pszIniFile, arrayLength(e_pszIniFile));
		PathRemoveFileSpec(e_pszIniFile);
		PathAppend(e_pszIniFile, _T("Clavier.ini"));
	}
	
	if (!e_hdlgModal) {
		if (new_ini_file) {
			shortcut::loadShortcuts();
		}
		
		for (int ini_file = 0; ini_file < mergeable_ini_files_count; ini_file++) {
			shortcut::mergeShortcuts(mergeable_ini_files[ini_file]);
			delete [] mergeable_ini_files[ini_file];
		}
		
		shortcut::saveShortcuts();
	}
	
	if (cmdopt == cmdoptNone) {
		cmdopt = (normal_launch)
			? ((auto_quit) ? cmdoptQuit : cmdoptLaunch)
			: ((auto_quit) ? cmdoptLaunch : cmdoptSettings);
	}
	
	return cmdopt;
}


void processCmdLineAction(CMDLINE_OPTION cmdopt) {
	UINT command;
	switch (cmdopt) {
		
		case cmdoptSettings:
			command = 0;
			break;
		
		case cmdoptMenu:
			PostMessage(e_hwndInvisible, msgClavierNotifyIcon, 0, WM_RBUTTONDOWN);
			return;
		
		case cmdoptQuit:
			PostMessage(e_hwndInvisible, msgClavierNotifyIcon, IDCCMD_QUIT, 0);
			return;
		
		case cmdoptAddText:
			command = ID_ADD_TEXT;
			break;
		
		case cmdoptAddCommand:
			command = ID_ADD_COMMAND;
			break;
		
		default:
			return;
	}
	PostMessage(e_hwndInvisible, msgClavierNotifyIcon, command, WM_LBUTTONDOWN);
}



// Invisible window:
// - quit when destroyed
// - handle traybar icon notifications
LRESULT CALLBACK prcInvisible(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (message == WM_DESTROY) {
Destroy:
		PostQuitMessage(0);
		
	} else if (message == msgClavierNotifyIcon) {
		if (wParam == 1 || wParam == IDCCMD_QUIT) {
			goto Destroy;
		}
		
		if (lParam != WM_LBUTTONDOWN && lParam != WM_RBUTTONDOWN) {
			return 0;
		}
		
		if (e_hdlgModal) {
			SetForegroundWindow(e_hdlgModal);
			if (e_hdlgModal == dialogs::e_hdlgMain && wParam) {
				PostMessage(dialogs::e_hdlgMain, WM_COMMAND, wParam, 0);
			}
			return 0;
		}
		
		if (lParam == WM_LBUTTONDOWN) {
			// Left click: display dialog box
			
			for (;;) {
				HeapCompact(e_hHeap, 0);
				switch (dialogs::showMainDialogModal(static_cast<UINT>(wParam))) {
					case IDCCMD_LANGUAGE:
						break;
					case IDCCMD_QUIT:
						goto Destroy;
					default:
						HeapCompact(e_hHeap, 0);
						return 0;
				}
			}
			
		} else /* (lParam == WM_RBUTTONDOWN)*/ {
			// Right click: display tray menu
			
			if (displayTrayIconMenu() == ID_TRAY_QUIT) {
				goto Destroy;
			}
		}
		
	} else if (message == msgTaskbarCreated) {
		updateTrayIcon(NIM_ADD);
	
	} else if (message == WM_COPYDATA) {
		// Execute command line
		
		shortcut::GuardList guard;
		const COPYDATASTRUCT& cds = *reinterpret_cast<const COPYDATASTRUCT*>(lParam);
		if (cds.dwData == static_cast<ULONG_PTR>(is_unicode)) {
			processCmdLineAction(execCmdLine((LPCTSTR)cds.lpData, false));
		}
		return TRUE;
	}
	
	return DefWindowProc(hwnd, message, wParam, lParam);
}



// Handle the tray icon
void updateTrayIcon(DWORD message) {
	VERIFV(e_bIconVisible);
	
	NOTIFYICONDATA nid;
	ZeroMemory(&nid, sizeof(nid));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = e_hwndInvisible;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = msgClavierNotifyIcon;
	if (message != NIM_DELETE) {
		nid.hIcon = (HICON)LoadImage(e_hInst, MAKEINTRESOURCE(IDI_APP), IMAGE_ICON, 16,16, 0);
		lstrcpy(nid.szTip, pszApp);
	}
	Shell_NotifyIcon(message, &nid);
}


UINT displayTrayIconMenu() {
	shortcut::GuardList guard;
	const HWND hwnd = e_hwndInvisible;
	
	const HMENU all_menus = i18n::loadMenu(IDM_CONTEXT);
	const HMENU menu = GetSubMenu(all_menus, 2);
	SetMenuDefaultItem(menu, ID_TRAY_SETTINGS, MF_BYCOMMAND);
	
	// Append INI file items
	
	// 1) Add current INI file
	LPTSTR ini_files[maxIniFile];
	ini_files[0] = e_pszIniFile;
	UINT ini_files_count = 1;
	
	// 2) Ad INI files from Clavier+ directory
	TCHAR ini_files_spec[MAX_PATH];
	GetModuleFileName(e_hInst, ini_files_spec, arrayLength(ini_files_spec));
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
				if (lstrcmpi(path, e_pszIniFile)) {
					ini_files[ini_files_count++] = path;
					if (ini_files_count == maxIniFile) {
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
	UINT id = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY,
		cursor_pos.x, cursor_pos.y, 0, hwnd, NULL);
	PostMessage(hwnd, WM_NULL, 0, 0);
	DestroyMenu(all_menus);
	
	switch (id) {
		case ID_TRAY_SETTINGS:
			PostMessage(hwnd, msgClavierNotifyIcon, 0, WM_LBUTTONDOWN);
			break;
		
		case ID_TRAY_COPYLIST:
			{
				String str;
				for (const Shortcut* shortcut = shortcut::getFirst(); shortcut;
						shortcut = shortcut->getNext()) {
					shortcut->appendCsvLineToString(str);
				}
				setClipboardText(str);
			}
			break;
		
		case ID_TRAY_INI_LOAD:
		case ID_TRAY_INI_MERGE:
		case ID_TRAY_INI_SAVE:
			{
				// Load the filters, replace '|' with '\0'
				TCHAR filters[bufString];
				i18n::loadStringAuto(IDS_INI_FILTER, filters);
				for (UINT chr_index = 0; filters[chr_index]; chr_index++) {
					if (filters[chr_index] == _T('|')) {
						filters[chr_index] = _T('\0');
					}
				}
				
				TCHAR ini_file[arrayLength(e_pszIniFile)];
				lstrcpy(ini_file, e_pszIniFile);
				
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
						lstrcpy(e_pszIniFile, ini_file);
						shortcut::saveShortcuts();
					}
				} else {
					ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
					if (GetOpenFileName(&ofn)) {
						if (id == ID_TRAY_INI_LOAD) {
							lstrcpy(e_pszIniFile, ini_file);
							shortcut::loadShortcuts();
						} else {
							shortcut::mergeShortcuts(ini_file);
						}
						shortcut::saveShortcuts();
					}
				}
			}
			break;
		
		default:
			if (id > ID_TRAY_INI_FIRSTFILE) {
				// INI file different than the current one
				lstrcpy(e_pszIniFile, ini_files[id - ID_TRAY_INI_FIRSTFILE]);
				shortcut::loadShortcuts();
			}
			break;
	}
	
	for (UINT ini_file = 1; ini_file < ini_files_count; ini_file++) {
		delete [] ini_files[ini_file];
	}
	
	return id;
}

}  // app namespace



LPCTSTR getToken(int tok) {
	return app::s_asToken[tok].get();
}

LPCTSTR getLanguageName(int lang) {
	return app::s_asToken[tokLanguageName].get(lang);
}


int findToken(LPCTSTR token) {
	for (int tok = 0; tok < arrayLength(app::s_asToken); tok++) {
		for (int lang = 0; lang < i18n::langCount; lang++) {
			if (!lstrcmpi(token, app::s_asToken[tok].get(lang))) {
				return tok;
			}
		}
	}
	return tokNotFound;
}
