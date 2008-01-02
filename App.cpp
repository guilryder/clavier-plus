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
const bool bUnicode = true;
#else
const bool bUnicode = false;
#endif


const LPCTSTR pszClavierWindow = _T("RyderClavierWindow");

static UINT msgTaskbarCreated;
static UINT msgClavierNotifyIcon;

const int maxIniFile = 20;

static TranslatedString s_asToken[tokNotFound];


enum CMDLINE_OPTION {
	cmdoptLaunch,
	cmdoptSettings,
	cmdoptMenu,
	cmdoptQuit,
	cmdoptAddText,
	cmdoptAddCommand,
	
	cmdoptWithArg,
	cmdoptLoad = cmdoptWithArg,
	cmdoptMerge,
	cmdoptSendKeys,
	
	cmdoptNone
};

static void initializeLanguages();
static void runGui(CMDLINE_OPTION cmdoptAction);
static CMDLINE_OPTION execCmdLine(LPCTSTR pszCmdLine, bool bNormalLaunch);
static void processCmdLineAction(CMDLINE_OPTION cmdoptAction);

static LRESULT CALLBACK prcInvisible(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static void updateTrayIcon(DWORD dwMessage);

// Displays the tray icon menu, as a child of the invisible window.
//
// Returns:
//   The ID of the chosen command, 0 if the user cancelled.
static UINT displayTrayIconMenu();


#ifdef _DEBUG
static void WinMainCRTStartup();

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	WinMainCRTStartup();
	return 0;
}
#endif  // _DEBUG


// Entry point
void WinMainCRTStartup() {
	msgTaskbarCreated = RegisterWindowMessage(_T("TaskbarCreated"));
	msgClavierNotifyIcon = RegisterWindowMessage(_T("RyderClavierOptions"));
	
	const LPCTSTR pszCmdLine = GetCommandLine();
	
#ifndef ALLOW_MULTIPLE_INSTANCES
	// Avoid multiple launching
	// Transmit the command line to the previous instance, if any
	{
		const HWND hwnd = FindWindow(_T("STATIC"), pszClavierWindow);
		if (hwnd) {
			COPYDATASTRUCT cds;
			cds.dwData = bUnicode;
			cds.cbData = (lstrlen(pszCmdLine) + 1) * sizeof(TCHAR);
			cds.lpData = (void*)pszCmdLine;
			SendMessage(hwnd, WM_COPYDATA, 0, (LPARAM)&cds);
			
#ifdef _DEBUG
			return;
#else
			ExitProcess(0);
#endif  // _DEBUG
		}
	}
#endif  // ALLOW_MULTIPLE_INSTANCES
	
	CoInitialize(NULL);
	
	e_hInst = (HINSTANCE)GetModuleHandle(NULL);
	e_hHeap = GetProcessHeap();
	e_hdlgModal = NULL;
	
	initializeLanguages();
	
	keyboard_hook::install();
	
	const CMDLINE_OPTION cmdoptAction = execCmdLine(pszCmdLine, true);
	if (cmdoptAction != cmdoptQuit) {
		runGui(cmdoptAction);
	}
	
	keyboard_hook::uninstall();
	
	CoUninitialize();
#ifndef _DEBUG
	ExitProcess(0);
#endif  // _DEBUG
}

void initializeLanguages() {
	TCHAR pszTokens[512];
	for (int lang = 0; lang < i18n::langCount; lang++) {
		i18n::setLanguage(lang);
		
		// Load all tokens in the current language.
		i18n::loadStringAuto(IDS_TOKENS, pszTokens);
		TCHAR *pcStart = pszTokens;
		for (int iToken = 0; iToken < tokNotFound; iToken++) {
			TCHAR *pc = pcStart;
			while (*pc != _T(';')) {
				pc++;
			}
			*pc = _T('\0');
			s_asToken[iToken].set(pcStart);
			pcStart = pc + 1;
		}
		
		dialogs::initializeCurrentLanguage();
	}
	
	i18n::setLanguage(i18n::getDefaultLanguage());
}

void runGui(CMDLINE_OPTION cmdoptAction) {
	// Create the invisible window
	e_hwndInvisible = CreateWindow(_T("STATIC"),
		pszClavierWindow, 0, 0,0,0,0, NULL, NULL, e_hInst, NULL);
	SubclassWindow(e_hwndInvisible, prcInvisible);
	
	// Create the traybar icon
	updateTrayIcon(NIM_ADD);
	
	processCmdLineAction(cmdoptAction);
	
	// Message loop
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	// Delete traybar icon
	updateTrayIcon(NIM_DELETE);
	
	// Save shortcuts, then delete them
	shortcut::saveShortcuts();
	shortcut::clearShortcuts();
}


static const LPCTSTR apszCmdOpt[] = {
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



// bLaunching is FALSE if the command line is sent by WM_COPYDATA.
// Return the action to execute.
CMDLINE_OPTION execCmdLine(LPCTSTR pszCmdLine, bool bNormalLaunch) {
	bool bNewIniFile = false;
	
	bool bInQuote;
	const TCHAR *pc = pszCmdLine;
	TCHAR chr;
	
	// Skip program name
	bInQuote = false;
	do {
		chr = *pc++;
		if (chr == _T('"')) {
			bInQuote ^= true;
		}
	} while (chr && (bInQuote || (chr != _T(' ') && chr != _T('\t'))));
	
	bInQuote = false;
	
	CMDLINE_OPTION cmdoptAction = cmdoptNone;
	bool bAutoQuit = false;
	LPTSTR apszIniFileMerge[maxIniFile];
	int nbIniFileMerge = 0;
	
	int nbArg = 0;
	if (chr) {
		// Parse arguments
		
		CMDLINE_OPTION cmdopt = cmdoptNone;
		
		const LPTSTR pszArg = new TCHAR[lstrlen(pszCmdLine) + 1];
		
		for (;;) {
			
			// Skip argument separator
			while (*pc == _T(' ') || *pc == '\t') {
				pc++;
			}
			if (!*pc) {
				break;
			}
			
			nbArg++;
			TCHAR *pcArg = pszArg;
			for (;;) {
				bool bCopyChar = true;
				
				// Count backslashes
				int nbBackslash = 0;
				while (*pc == _T('\\')) {
					pc++;
					nbBackslash++;
				}
				
				// Process quoting
				if (*pc == _T('"')) {
					if (nbBackslash % 2 == 0) {
						if (bInQuote && pc[1] == _T('"')) {
							pc++;
						} else {
							bCopyChar = false;
							bInQuote ^= true;
						}
					}
					nbBackslash /= 2;
				}
				
				while (nbBackslash--) {
					*pcArg++ = _T('\\');
				}
				
				// Stop on spaces
				chr = *pc;
				if (!chr || (!bInQuote && (chr == _T(' ') || chr == _T('\t')))) {
					break;
				}
				
				if (bCopyChar) {
					*pcArg++ = chr;
				}
				pc++;
			}
			*pcArg = _T('\0');
			
			if (cmdopt == cmdoptNone) {
				// Test for command line option
				if (*pszArg == _T('/')) {
					const LPCTSTR pszOption = pszArg + 1;
					for (cmdopt = (CMDLINE_OPTION)0; cmdopt < cmdoptNone;
							cmdopt = (CMDLINE_OPTION)(cmdopt + 1)) {
						if (!lstrcmpi(pszOption, apszCmdOpt[cmdopt])) {
							break;
						}
					}
				}
				
				if (cmdopt >= cmdoptWithArg) {
					continue;
				}
			}
			
			switch (cmdopt) {
				
				// Set INI filename
				case cmdoptLoad:
					bAutoQuit = false;
					bNewIniFile = true;
					lstrcpyn(e_pszIniFile, pszArg, nbArray(e_pszIniFile));
					break;
				
				// Merge an INI file
				case cmdoptMerge:
					bAutoQuit = false;
					if (nbIniFileMerge < nbArray(apszIniFileMerge)) {
						const LPTSTR pszIniFile = new TCHAR[lstrlen(pszArg) + 1];
						lstrcpy(pszIniFile, pszArg);
						apszIniFileMerge[nbIniFileMerge++] = pszIniFile;
					}
					break;
				
				// Send keys
				case cmdoptSendKeys:
					bAutoQuit = true;
					{
						Keystroke ks;
						Shortcut sh(ks);
						sh.m_bCommand = false;
						sh.m_sText = pszArg;
						sh.execute(false);
					}
					break;
				
				// Other action
				default:
					cmdoptAction = cmdopt;
					bAutoQuit = false;
					break;
			}
			
			cmdopt = cmdoptNone;
		}
		
		delete [] pszArg;
	}
	
	// Default INI file
	if (bNormalLaunch && !bNewIniFile && !bAutoQuit) {
		bNewIniFile = true;
		GetModuleFileName(e_hInst, e_pszIniFile, nbArray(e_pszIniFile));
		PathRemoveFileSpec(e_pszIniFile);
		PathAppend(e_pszIniFile, _T("Clavier.ini"));
	}
	
	if (!e_hdlgModal) {
		if (bNewIniFile) {
			shortcut::loadShortcuts();
		}
		
		for (int i = 0; i < nbIniFileMerge; i++) {
			shortcut::mergeShortcuts(apszIniFileMerge[i]);
			delete [] apszIniFileMerge[i];
		}
		
		shortcut::saveShortcuts();
	}
	
	if (cmdoptAction == cmdoptNone) {
		cmdoptAction = (bNormalLaunch)
			? ((bAutoQuit) ? cmdoptQuit : cmdoptLaunch)
			: ((bAutoQuit) ? cmdoptLaunch : cmdoptSettings);
	}
	
	return cmdoptAction;
}


void processCmdLineAction(CMDLINE_OPTION cmdoptAction) {
	UINT command;
	switch (cmdoptAction) {
		
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
LRESULT CALLBACK prcInvisible(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_DESTROY) {
Destroy:
		PostQuitMessage(0);
		
	} else if (uMsg == msgClavierNotifyIcon) {
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
				switch (dialogs::showMainDialogModal(wParam)) {
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
		
	} else if (uMsg == msgTaskbarCreated) {
		updateTrayIcon(NIM_ADD);
	
	} else if (uMsg == WM_COPYDATA) {
		// Execute command line
		
		const COPYDATASTRUCT &cds = *(const COPYDATASTRUCT*)lParam;
		if (cds.dwData == (ULONG_PTR)bUnicode) {
			processCmdLineAction(execCmdLine((LPCTSTR)cds.lpData, false));
		}
		return TRUE;
	}
	
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}



// Handle the tray icon
void updateTrayIcon(DWORD dwMessage) {
	VERIFV(e_bIconVisible);
	
	NOTIFYICONDATA nid;
	ZeroMemory(&nid, sizeof(nid));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = e_hwndInvisible;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = msgClavierNotifyIcon;
	if (dwMessage != NIM_DELETE) {
		nid.hIcon = (HICON)LoadImage(e_hInst, MAKEINTRESOURCE(IDI_APP), IMAGE_ICON, 16,16, 0);
		lstrcpy(nid.szTip, pszApp);
	}
	Shell_NotifyIcon(dwMessage, &nid);
}


UINT displayTrayIconMenu() {
	const HWND hwnd = e_hwndInvisible;
	
	const HMENU hAllMenus = i18n::loadMenu(IDM_CONTEXT);
	const HMENU hMenu = GetSubMenu(hAllMenus, 2);
	SetMenuDefaultItem(hMenu, ID_TRAY_SETTINGS, MF_BYCOMMAND);
	
	// Append INI file items
	
	// 1) List current INI file
	LPTSTR apszIniFile[maxIniFile];
	apszIniFile[0] = e_pszIniFile;
	UINT nbIniFile = 1;
	
	// 2) List INI files in Clavier+ directory
	TCHAR pszIniFileSpec[MAX_PATH];
	GetModuleFileName(e_hInst, pszIniFileSpec, nbArray(pszIniFileSpec));
	PathRemoveFileSpec(pszIniFileSpec);
	PathAppend(pszIniFileSpec, _T("*.ini"));
	WIN32_FIND_DATA wfd;
	const HANDLE hff = FindFirstFile(pszIniFileSpec, &wfd);
	PathRemoveFileSpec(pszIniFileSpec);
	if (hff != INVALID_HANDLE_VALUE) {
		do {
			if (!(wfd.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN))) {
				const LPTSTR pszPath = new TCHAR[MAX_PATH];
				PathCombine(pszPath, pszIniFileSpec, wfd.cFileName);
				if (lstrcmpi(pszPath, e_pszIniFile)) {
					apszIniFile[nbIniFile++] = pszPath;
					if (nbIniFile == maxIniFile) {
						break;
					}
				} else {
					delete [] pszPath;
				}
			}
		} while (FindNextFile(hff, &wfd));
		FindClose(hff);
	}
	
	// 2) Append INI files to menu
	for (UINT iIniFile = 0; iIniFile < nbIniFile; iIniFile++) {
		InsertMenu(hMenu, iIniFile, MF_BYPOSITION | MF_STRING, ID_TRAY_INI_FIRSTFILE + iIniFile,
				PathFindFileName(apszIniFile[iIniFile]));
	}
	CheckMenuRadioItem(hMenu, ID_TRAY_INI_FIRSTFILE, ID_TRAY_INI_FIRSTFILE + nbIniFile - 1,
		ID_TRAY_INI_FIRSTFILE, MF_BYCOMMAND);
	
	// Show the popup menu
	POINT ptCursor;
	GetCursorPos(&ptCursor);
	SetForegroundWindow(hwnd);
	UINT id = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY,
		ptCursor.x, ptCursor.y, 0, hwnd, NULL);
	PostMessage(hwnd, WM_NULL, 0, 0);
	DestroyMenu(hAllMenus);
	
	switch (id) {
		case ID_TRAY_SETTINGS:
			PostMessage(hwnd, msgClavierNotifyIcon, 0, WM_LBUTTONDOWN);
			break;
		
		case ID_TRAY_COPYLIST:
			{
				String s;
				for (const Shortcut *psh = shortcut::e_pshFirst; psh; psh = psh->m_pNext) {
					psh->appendItemToString(s);
				}
				shortcut::copyShortcutsToClipboard(s);
			}
			break;
		
		case ID_TRAY_INI_LOAD:
		case ID_TRAY_INI_MERGE:
		case ID_TRAY_INI_SAVE:
			{
				TCHAR psz[bufString];
				i18n::loadStringAuto(IDS_INI_FILTER, psz);
				for (UINT i = 0; psz[i]; i++) {
					if (psz[i] == _T('|')) {
						psz[i] = _T('\0');
					}
				}
				
				TCHAR pszIniFile[nbArray(e_pszIniFile)];
				lstrcpy(pszIniFile, e_pszIniFile);
				
				OPENFILENAME ofn;
				ZeroMemory(&ofn, OPENFILENAME_SIZE_VERSION_400);
				ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
				ofn.hwndOwner = hwnd;
				ofn.lpstrFile = pszIniFile;
				ofn.nMaxFile = nbArray(pszIniFile);
				ofn.lpstrFilter = psz;
				if (id == ID_TRAY_INI_SAVE) {
					ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
					if (GetSaveFileName(&ofn)) {
						lstrcpy(e_pszIniFile, pszIniFile);
						shortcut::saveShortcuts();
					}
				} else {
					ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
					if (GetOpenFileName(&ofn)) {
						if (id == ID_TRAY_INI_LOAD) {
							lstrcpy(e_pszIniFile, pszIniFile);
							shortcut::loadShortcuts();
						} else {
							shortcut::mergeShortcuts(pszIniFile);
						}
						shortcut::saveShortcuts();
					}
				}
			}
			break;
		
		default:
			if (id > ID_TRAY_INI_FIRSTFILE) {
				// INI file different than the current one
				lstrcpy(e_pszIniFile, apszIniFile[id - ID_TRAY_INI_FIRSTFILE]);
				shortcut::loadShortcuts();
			}
			break;
	}
	
	for (UINT iIniFile = 1; iIniFile < nbIniFile; iIniFile++) {
		delete [] apszIniFile[iIniFile];
	}
	
	return id;
}



LPCTSTR getToken(int tok) {
	return s_asToken[tok].get();
}

LPCTSTR getLanguageName(int lang) {
	return s_asToken[tokLanguageName].get(lang);
}


int findToken(LPCTSTR pszToken) {
	for (int tok = 0; tok < nbArray(s_asToken); tok++) {
		for (int lang = 0; lang < i18n::langCount; lang++) {
			if (!lstrcmpi(pszToken, s_asToken[tok].get(lang))) {
				return tok;
			}
		}
	}
	return tokNotFound;
}


void skipUntilComma(TCHAR*& rpc, bool bUnescape) {
	TCHAR *const pcStart = rpc;
	bool bEscaping = false;
	TCHAR *pc = pcStart;
	while (*pc) {
		if (bEscaping) {
			bEscaping = false;
		} else if (*pc == _T(',')) {
			break;
		} else if (bUnescape && *pc == _T('\\')) {
			bEscaping = true;
		}
		pc++;
	}
	if (*pc) {
		*pc++ = _T('\0');
		while (*pc == _T(' ')) {
			pc++;
		}
	}
	rpc = pc;
	
	if (bUnescape) {
		const TCHAR *pcIn = pcStart;
		TCHAR *pcOut = pcStart;
		bool bEscaping = false;
		while (*pcIn) {
			if (*pcIn == _T('\\') && !bEscaping) {
				bEscaping = true;
			} else{
				bEscaping = false;
				*pcOut++ = *pcIn;
			}
			pcIn++;
		}
		*pcOut = _T('\0');
	}
}
