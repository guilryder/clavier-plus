// Clavier+
// Keyboard shortcuts manager
//
// Copyright (C) 2000-2006 Guillaume Ryder
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
#include "Clavier.h"


const LPCTSTR pszValueAutoStart = pszApp;

const LPCTSTR pszDonateURL = _T("https://www.paypal.com/cgi-bin/webscr?cmd=_xclick&business=gryder%40club%2dinternet%2efr&item_name=UtilFr%20%2d%20Clavier%2b&no_shipping=2&no_note=1&tax=0&currency_code=EUR&bn=PP%2dDonationsBF&charset=UTF%2d8");

static UINT msgTaskbarCreated;
static UINT msgClavierNotifyIcon;


enum
{
	idSettings = 1,
	idCopyList,
	idQuit,
	idFirstShortcut,
};

const UINT nbRowMenu = 25;


static const int s_aiShow[] = 
{
	SW_NORMAL, SW_MINIMIZE, SW_MAXIMIZE,
};


enum
{
	colShortcut,
	colCond,
	colType,
	colContents,
	colDescription,
	colCount
};

const int nbColSize = colCount - 1;

static const int s_acxColOrig[] = { 25, 10, 12, 40 };
static int s_acxCol[colCount];


static const int s_avkCond[] =
{
	VK_CAPITAL, VK_NUMLOCK, VK_SCROLL,
};


static TranslatedString s_asToken[tokNotFound];
static TranslatedString s_sLanguageName;
static TranslatedString s_sCondKeys;


static HWND s_hdlgModal = NULL;
       HWND e_hlst = NULL;      // Shortcut list
static Shortcut *s_psh;         // Current edited shortcut
       Shortcut *e_pshFirst;    // Shortcut list

static bool s_bCanEvent;

// Last known foreground window
// Used by shortcuts menu, to send keys to another window
// than the traybar or Clavier+ invisible window.
static HWND s_hwndLastForeground = NULL;


static LRESULT CALLBACK prcInvisible(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL    CALLBACK prcMain     (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL    CALLBACK prcKeystroke(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL    CALLBACK prcLanguage (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL    CALLBACK prcAbout    (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


static void trayIconAction(DWORD dwMessage);

static void updateItem();
static void updateList(HWND hDlg);
static int  addItem(Shortcut* psh);

static void iniGetPath(LPTSTR pszIniFile);
static void iniSave();
static void iniLoad();
static HKEY openAutoStartKey(LPTSTR pszPath);


static Shortcut* findShortcut(BYTE vk, WORD vkFlags, const int aiCondState[]);
static Shortcut* getSelShortcut();

static void copyShortcutsListToClipboard();


#ifdef _DEBUG
static void WinMainCRTStartup();

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	WinMainCRTStartup();
	return 0;
}
#endif


// Entry point
void WinMainCRTStartup()
{
	e_hHeap = GetProcessHeap();
	
	msgTaskbarCreated = RegisterWindowMessage(_T("TaskbarCreated"));
	msgClavierNotifyIcon = RegisterWindowMessage(_T("RyderClavierOptions"));
	
	
#ifndef _DEBUG
	// Avoid multiple launching
	// Show the main dialog box if already launched
	CreateMutex(NULL, TRUE, pszApp);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		SendMessage(HWND_BROADCAST, msgClavierNotifyIcon, 0, WM_LBUTTONDOWN);
		return;
	}
#endif
	
	e_hInst = (HINSTANCE)GetModuleHandle(NULL);
	
	{
		const LCID lcidOld = GetThreadLocale();
		
		for (int lang = 0; lang < langCount; lang++) {
			setResourceLanguage(lang);
			
			// Load tokens
			static TCHAR pszTokens[512];
			loadStringAuto(IDS_TOKENS, pszTokens);
			TCHAR *pcStart = pszTokens;
			for (int iToken = 0; iToken < tokNotFound; iToken++) {
				TCHAR *pc = pcStart;
				while (*pc != _T(';'))
					pc++;
				*pc = 0;
				s_asToken[iToken].set(pcStart);
				pcStart = pc + 1;
			}
			
			s_sLanguageName.load(IDS_LANGUAGE);
			
			s_sCondKeys.load(IDS_CONDITION_KEYS);
		}
		
		int lang = langUS;
		switch (PRIMARYLANGID(LANGIDFROMLCID(lcidOld))) {
			case LANG_FRENCH:  lang = langFR;  break;
			case LANG_GERMAN:  lang = langDE;  break;
		}
		setResourceLanguage(lang);
	}
	
	
	// Load the INI file
	iniLoad();
	
	// Create the invisible window
	e_hwndInvisible = CreateWindow(_T("STATIC"), NULL, 0,
		0,0,0,0, NULL, NULL, e_hInst, NULL);
	SetWindowLong(e_hwndInvisible, GWL_WNDPROC, (LONG)prcInvisible);
	
	// Create the traybar icon
	trayIconAction(NIM_ADD);
	
#ifdef _DEBUG
	PostMessage(e_hwndInvisible, msgClavierNotifyIcon, 0, WM_LBUTTONDOWN);
#endif
	
	// Message loop
	DWORD timeMinimum = 0;
	DWORD timeLast = GetTickCount();
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		const DWORD timeCurrent = GetMessageTime();
		if (msg.message == WM_HOTKEY) {
			// Shortcut!
			
			if (timeCurrent < timeLast)
				timeMinimum = 0;
			if (timeCurrent < timeMinimum)
				goto Next;
			
			int aCondState[condTypeCount];
			for (int i = 0; i < condTypeCount; i++)
				aCondState[i] = (GetKeyState(s_avkCond[i]) & 0x01) ? condYes : condNo;
			Shortcut *const psh = findShortcut(
				(BYTE)HIWORD(msg.lParam), LOWORD(msg.lParam), aCondState);
			if (psh) {
				if (psh->execute())
					timeMinimum = GetTickCount();
			}else{
				Keystroke ks;
				ks.m_vk      = (BYTE)HIWORD(msg.lParam);
				ks.m_vkFlags = LOWORD(msg.lParam);
				HWND hwndFocus;
				Keystroke::catchKeyboardFocus(hwndFocus);
				ks.unregisterHotKey();
				ks.simulateTyping();
				ks.registerHotKey();
			}
			
		}else{
			// Other message
			
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		
Next:
		timeLast = timeCurrent;
	}
	
	// Delete traybar icon
	trayIconAction(NIM_DELETE);
	
	// Save shortcuts, then delete them
	iniSave();
	while (e_pshFirst) {
		Shortcut *const psh = e_pshFirst;
		e_pshFirst = psh->m_pNext;
		delete psh;
	}
	
	ExitProcess(0);
}


// Invisible window for the traybar icon
LRESULT CALLBACK prcInvisible(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_DESTROY) {
Destroy:
		PostQuitMessage(0);
		
	}else if (uMsg == msgClavierNotifyIcon) {
		if (wParam != 0)
			goto Destroy;
		
		const HWND hwndForeground = GetForegroundWindow();
		TCHAR pszClass[128];
		GetClassName(hwndForeground, pszClass, nbArray(pszClass));
		if (lstrcmp(pszClass, _T("Shell_TrayWnd")) && hwndForeground != hWnd)
			s_hwndLastForeground = hwndForeground;
		
		if (lParam != WM_LBUTTONDOWN && lParam != WM_RBUTTONDOWN)
			return 0;
		
		if (s_hdlgModal) {
			SetForegroundWindow(s_hdlgModal);
			return 0;
		}
		
		if (lParam == WM_LBUTTONDOWN) {
			// Left click: display dialog box
			
			for (;;) {
				switch (DialogBox(e_hInst, MAKEINTRESOURCE(IDD_MAIN), e_hwndInvisible, prcMain)) {
					case IDCCMD_LANGUAGE:
						DialogBox(e_hInst, MAKEINTRESOURCE(IDD_LANGUAGE), e_hwndInvisible, prcLanguage);
						break;
					case IDCCMD_QUIT:
						goto Destroy;
					default:
						return 0;
				}
			}
		}else if (lParam == WM_RBUTTONDOWN) {
			// Right click: display shortcuts menu
			
			const HMENU hMenu = CreatePopupMenu();
			
			// Add shortcut items
			UINT id = idFirstShortcut;
			for (const Shortcut *psh = e_pshFirst; psh; psh = psh->m_pNext)
				psh->appendMenuItem(hMenu, id++);
			
			// Add separator, Show, and Quit
			if (e_pshFirst)
				AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
			
			TCHAR psz[bufString];
			AppendMenu(hMenu, MF_STRING, idSettings, loadStringAutoRet(IDS_SETTINGS, psz));
			AppendMenu(hMenu, MF_STRING, idCopyList, loadStringAutoRet(IDS_COPYLIST, psz));
			AppendMenu(hMenu, MF_STRING, idQuit,     loadStringAutoRet(IDS_QUIT, psz));
			SetMenuDefaultItem(hMenu, idSettings, MF_BYCOMMAND);
			
			// Show the popup menu
			POINT ptCursor;
			GetCursorPos(&ptCursor);
			SetForegroundWindow(hWnd);
			id = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY,
				ptCursor.x, ptCursor.y, 0, hWnd, NULL);
			PostMessage(hWnd, WM_NULL, 0, 0);
			
			if (id >= idFirstShortcut) {
				// Shortcut
				
				id -= idFirstShortcut;
				for (const Shortcut *psh = e_pshFirst; psh; psh = psh->m_pNext)
					if (id-- == 0) {
						SetForegroundWindow(s_hwndLastForeground);
						psh->execute();
						break;
					}
				
			}else if (id == idSettings)
				PostMessage(hWnd, msgClavierNotifyIcon, 0, WM_LBUTTONDOWN);
			
			else if (id == idCopyList)
				copyShortcutsListToClipboard();
			
			else if (id == idQuit)
				goto Destroy;
		}
		
	}else if (uMsg == msgTaskbarCreated)
		trayIconAction(NIM_ADD);
	
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


enum
{
	sizeposLeft      = 1 << 0,
	sizeposLeftHalf  = 1 << 1,
	sizeposWidth     = 1 << 2,
	sizeposWidthHalf = 1 << 3,
	sizeposTop       = 1 << 4,
	sizeposHeight    = 1 << 5,
};

struct SIZEPOS
{
	int id;
	int flags;
	
	HWND  hctl;
	POINT pt;
	SIZE  size;
	
	
	void updateGui(int dx, int dy)
	{
		POINT ptNew = pt;
		SIZE sizeNew = size;
		
		if (flags & sizeposLeft)
			ptNew.x += dx;
		else if (flags & sizeposLeftHalf)
			ptNew.x += dx / 2;
		
		if (flags & sizeposWidth)
			sizeNew.cx += dx;
		else if (flags & sizeposWidthHalf)
			sizeNew.cx += dx / 2;
		
		if (flags & sizeposTop)
			ptNew.y += dy;
		
		if (flags & sizeposHeight)
			sizeNew.cy += dy;
		
		MoveWindow(hctl, ptNew.x, ptNew.y, sizeNew.cx, sizeNew.cy, FALSE);
	}
};

static SIZEPOS aSizePos[] =
{
	{ IDCLST, sizeposWidth | sizeposHeight },
	
	{ IDCCMD_ADD,       sizeposTop },
	{ IDCCMD_DELETE,    sizeposTop },
	{ IDCCMD_EDIT,      sizeposTop },
	{ IDCLBL_HELP,      sizeposTop },
	{ IDCCHK_AUTOSTART, sizeposTop },
	
	{ IDCOPT_TEXT, sizeposTop },
	{ IDCFRA_TEXT, sizeposTop | sizeposWidthHalf },
	{ IDCTXT_TEXT, sizeposTop | sizeposWidthHalf },
	
	{ IDCOPT_COMMAND,   sizeposTop | sizeposLeftHalf },
	{ IDCFRA_COMMAND,   sizeposTop | sizeposLeftHalf | sizeposWidthHalf },
	{ IDCTXT_COMMAND,   sizeposTop | sizeposLeftHalf | sizeposWidthHalf },
	{ IDCTXT_DIRECTORY, sizeposTop | sizeposLeftHalf | sizeposWidthHalf },
	{ IDCCBO_SHOW,      sizeposTop | sizeposLeftHalf | sizeposWidthHalf },
	{ IDCLBL_DIRECTORY, sizeposTop | sizeposLeftHalf },
	{ IDCLBL_SHOW,      sizeposTop | sizeposLeftHalf },
	{ IDCCMD_BROWSE,    sizeposTop | sizeposLeft },
	{ IDCCMD_BROWSEDIR, sizeposTop | sizeposLeft },
	{ IDCLBL_ICON,      sizeposTop | sizeposLeft },
	
	{ IDCLBL_DESCRIPTION, sizeposTop },
	{ IDCTXT_DESCRIPTION, sizeposTop | sizeposWidth },
	
	{ IDCLBL_DONATE, sizeposTop },
	
	{ IDCCMD_README,   sizeposTop | sizeposLeft },
	{ IDCCMD_COPYLIST, sizeposTop | sizeposLeft },
	{ IDCCMD_LANGUAGE, sizeposTop | sizeposLeft },
	{ IDCCMD_ABOUT,    sizeposTop | sizeposLeft },
	{ IDCCMD_QUIT,     sizeposTop | sizeposLeft },
	{ IDCANCEL,        sizeposTop | sizeposLeft },
};

static SIZE s_sizeMainDialog = { 0, 0 };


// Main dialog box
BOOL CALLBACK prcMain(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static RECT rcClientOrig;
	static SIZE sizeMinimum;
	
	switch (uMsg) {
		
		// Initialization
		case WM_INITDIALOG:
			{
				GetClientRect(hDlg, &rcClientOrig);
				getWindowSize(hDlg, sizeMinimum);
				
				for (int i = 0; i < nbArray(aSizePos); i++) {
					SIZEPOS &sp = aSizePos[i];
					sp.hctl = GetDlgItem(hDlg, sp.id);
					RECT rc;
					GetWindowRect(sp.hctl, &rc);
					sp.size.cx = rc.right  - rc.left;
					sp.size.cy = rc.bottom - rc.top;
					ScreenToClient(hDlg, (POINT*)&rc);
					sp.pt = (POINT&)rc;
				}
				
				initializeWebLink(hDlg, IDCLBL_DONATE, pszDonateURL);
				
				s_bCanEvent = false;
				
				s_hdlgModal = hDlg;
				e_hlst = GetDlgItem(hDlg, IDCLST);
				ListView_SetExtendedListViewStyle(e_hlst, LVS_EX_FULLROWSELECT);
				
				// Add columns to list
				TCHAR pszColumns[512];
				loadStringAuto(IDS_COLUMNS, pszColumns);
				TCHAR *pcColumn = pszColumns;
				LVCOLUMN lvc;
				lvc.mask = LVCF_TEXT | LVCF_FMT | LVCF_SUBITEM;
				for (lvc.iSubItem = 0; lvc.iSubItem < colCount; lvc.iSubItem++) {
					lvc.pszText = pcColumn;
					while (*pcColumn != _T(';'))
						pcColumn++;
					*pcColumn++ = 0;
					lvc.fmt = (lvc.iSubItem == 1) ? LVCFMT_CENTER : LVCFMT_LEFT;
					ListView_InsertColumn(e_hlst, lvc.iSubItem, &lvc);
				}
				
				// Fill shortcuts list and unregister hot keys
				for (Shortcut *psh = e_pshFirst; psh; psh = psh->m_pNext) {
					psh->unregisterHotKey();
					addItem(psh);
				}
				ListView_SortItems(e_hlst, Shortcut::compare, NULL);
				
				updateList(hDlg);
				
				// Button icons
				for (int i = 0; i < 3; i++)
					SendDlgItemMessage(hDlg, IDCCMD_ADD + i, BM_SETIMAGE, IMAGE_ICON,
						(LPARAM)(HICON)LoadImage(e_hInst, MAKEINTRESOURCE(IDI_ADD + i), IMAGE_ICON, 15,15, 0));
				
				// Auto start
				TCHAR pszAutoStartPath[MAX_PATH];
				const HKEY hKeyAutoStart = openAutoStartKey(pszAutoStartPath);
				if (hKeyAutoStart) {
					DWORD dwType, dwLen;
					TCHAR pszPath[MAX_PATH];
					
					if (ERROR_SUCCESS == RegQueryValueEx(hKeyAutoStart, pszValueAutoStart, NULL,
					    &dwType, NULL, &dwLen) && dwType == REG_SZ && dwLen < nbArray(pszPath))
						if (ERROR_SUCCESS == RegQueryValueEx(hKeyAutoStart, pszValueAutoStart, NULL,
						    NULL, (BYTE*)pszPath, &dwLen) && !lstrcmpi(pszPath, pszAutoStartPath))
							CheckDlgButton(hDlg, IDCCHK_AUTOSTART, TRUE);
					
					RegCloseKey(hKeyAutoStart);
				}
				
				// Fill "command line show" list
				for (int i = 0; i < nbArray(s_aiShow); i++)
					SendDlgItemMessage(hDlg, IDCCBO_SHOW, CB_ADDSTRING, 0, (LPARAM)s_asToken[tokShowNormal + i].get());
				
				s_bCanEvent = true;
				
				SendMessage(hDlg, WM_SETICON, FALSE,
					(LPARAM)(HICON)LoadImage(e_hInst, MAKEINTRESOURCE(IDI_APP), IMAGE_ICON, 16,16, 0));
				
				if (s_sizeMainDialog.cx < sizeMinimum.cx || s_sizeMainDialog.cy < sizeMinimum.cy)
					s_sizeMainDialog = sizeMinimum;
				else{
					HMONITOR hMonitor = MonitorFromWindow(hDlg, MONITOR_DEFAULTTOPRIMARY);
					MONITORINFO mi;
					mi.cbSize = sizeof(mi);
					GetMonitorInfo(hMonitor, &mi);
					SetWindowPos(hDlg, NULL,
						(mi.rcWork.left + mi.rcWork.right  - s_sizeMainDialog.cx) / 2,
						(mi.rcWork.top  + mi.rcWork.bottom - s_sizeMainDialog.cy) / 2,
						s_sizeMainDialog.cx, s_sizeMainDialog.cy, SWP_NOZORDER | SWP_NOACTIVATE);
				}
				
				SendMessage(hDlg, WM_SIZE, 0, 0);
			}
			return TRUE;
		
		
		// Command
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				
				case IDCCMD_QUIT:
					if (IDNO == messageBox(hDlg, ASK_QUIT, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION))
						break;
					
				case IDCANCEL:
					{
						// Create a valid linked list from the list box
						// and register the hot keys
						const int nbItem = ListView_GetItemCount(e_hlst);
						e_pshFirst = NULL;
						LVITEM lvi;
						lvi.mask = LVIF_PARAM;
						for (lvi.iItem = nbItem; --lvi.iItem >= 0;) {
							ListView_GetItem(e_hlst, &lvi);
							Shortcut *const psh = (Shortcut*)lvi.lParam;
							psh->registerHotKey();
							psh->m_pNext = e_pshFirst;
							e_pshFirst = psh;
						}
						
						// Create/delete the auto start key
						TCHAR pszAutoStartPath[MAX_PATH];
						const HKEY hKeyAutoStart = openAutoStartKey(pszAutoStartPath);
						if (hKeyAutoStart) {
							if (IsDlgButtonChecked(hDlg, IDCCHK_AUTOSTART))
								RegSetValueEx(hKeyAutoStart, pszValueAutoStart, NULL, REG_SZ,
									(const BYTE*)pszAutoStartPath, lstrlen(pszAutoStartPath) + 1);
							else
								RegDeleteValue(hKeyAutoStart, pszValueAutoStart);
							
							RegCloseKey(hKeyAutoStart);
						}
					}
					
					iniSave();
					s_hdlgModal = NULL;
					EndDialog(hDlg, LOWORD(wParam));
					break;
				
				case IDCCMD_ADD:
					{
						SetFocus(e_hlst);
						Keystroke ks;
						if (AskKeystroke(hDlg, NULL, ks)) {
							Shortcut *const psh = new Shortcut(ks);
							const int iItem = addItem(psh);
							ListView_SetItemState(e_hlst, iItem,
								LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
							ListView_SortItems(e_hlst, Shortcut::compare, NULL);
							updateList(hDlg);
							SetFocus(GetDlgItem(hDlg, IDCTXT_TEXT));
						}
					}
					break;
				
				case IDCCMD_DELETE:
					{
						if (IDNO == messageBox(hDlg, ASK_DELETE, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION))
							break;
						
						SetFocus(GetDlgItem(hDlg, IDCCMD_ADD));
						SendMessage(hDlg, WM_NEXTDLGCTL, TRUE, FALSE);
						
						LVITEM lvi;
						lvi.iItem = ListView_GetNextItem(e_hlst, -1, LVNI_SELECTED);
						if (lvi.iItem >= 0) {
							lvi.mask = LVIF_PARAM;
							ListView_GetItem(e_hlst, &lvi);
							
							// Delete item, hotkey, and shortcut object
							ListView_DeleteItem(e_hlst, lvi.iItem);
							((Shortcut*)lvi.lParam)->unregisterHotKey();
							delete (Shortcut*)lvi.lParam;
						}
						
						// Select an other item and update
						if (lvi.iItem >= ListView_GetItemCount(e_hlst))
							lvi.iItem--;
						ListView_SetItemState(e_hlst, lvi.iItem,
							LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
						updateList(hDlg);
						SetFocus(e_hlst);
					}
					break;
				
				case IDCCMD_EDIT:
					SetFocus(e_hlst);
					{
						Keystroke ks = *s_psh;
						if (AskKeystroke(hDlg, s_psh, ks)) {
							updateItem();
							ListView_SortItems(e_hlst, Shortcut::compare, NULL);
							updateList(hDlg);
						}
					}
					break;
				
				
				case IDCCMD_README:
					{
						TCHAR pszDir[MAX_PATH];
						GetModuleFileName(e_hInst, pszDir, nbArray(pszDir));
						PathRemoveFileSpec(pszDir);
						
						TCHAR pszFile[MAX_PATH];
						loadStringAuto(IDS_README, pszFile);
						
						ShellExecute(NULL, NULL, pszFile, NULL, pszDir, SW_SHOWDEFAULT);
					}
					break;
				
				
				case IDCCMD_LANGUAGE:
					EndDialog(hDlg, IDCCMD_LANGUAGE);
					break;
				
				
				case IDCCMD_ABOUT:
					DialogBox(e_hInst, MAKEINTRESOURCE(IDD_ABOUT), hDlg, prcAbout);
					break;
				
				
				case IDCCMD_COPYLIST:
					copyShortcutsListToClipboard();
					break;
				
				
				// - Type text
				// - Run command line
				case IDCOPT_TEXT:
				case IDCOPT_COMMAND:
					{
						const bool bCommand = (LOWORD(wParam)==IDCOPT_COMMAND);
						s_psh->m_bCommand = bCommand;
						updateList(hDlg);
						SetFocus(GetDlgItem(hDlg, (bCommand) ? IDCTXT_COMMAND : IDCTXT_TEXT));
					}
					break;
				
				
				// Text, command, directory, description
				case IDCTXT_TEXT:
				case IDCTXT_COMMAND:
				case IDCTXT_DIRECTORY:
				case IDCTXT_DESCRIPTION:
					if (HIWORD(wParam) == EN_CHANGE && s_bCanEvent) {
						String *ps;
						switch (LOWORD(wParam)) {
							case IDCTXT_TEXT:       ps = &s_psh->m_sText;         break;
							case IDCTXT_COMMAND:    ps = &s_psh->m_sCommand;      break;
							case IDCTXT_DIRECTORY:  ps = &s_psh->m_sDirectory;    break;
							default:                ps = &s_psh->m_sDescription;  break;
						}
						
						const int buf = GetWindowTextLength((HWND)lParam) + 1;
						GetWindowText((HWND)lParam, ps->getBuffer(buf), buf);
						
						updateItem();
					}
					break;
				
				// Browse for command line
				case IDCCMD_BROWSE:
					{
#ifndef OPENFILENAME_SIZE_VERSION_400
#define OPENFILENAME_SIZE_VERSION_400  sizeof(OPENFILENAME)
#endif
						
						// Get the command line file (without arguments)
						TCHAR pszFile[MAX_PATH];
						GetDlgItemText(hDlg, IDCTXT_COMMAND, pszFile, nbArray(pszFile));
						PathRemoveArgs(pszFile);
						PathUnquoteSpaces(pszFile);
						
						// Ask for the file
						OPENFILENAME ofn;
						ZeroMemory(&ofn, OPENFILENAME_SIZE_VERSION_400);
						ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
						ofn.hwndOwner   = hDlg;
						ofn.lpstrFile   = pszFile;
						ofn.nMaxFile    = nbArray(pszFile);
						ofn.Flags       = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
						if (!GetOpenFileName(&ofn))
							break;
						
						// Update internal structures
						PathQuoteSpaces(pszFile);
						SetDlgItemText(hDlg, IDCTXT_COMMAND, pszFile);
						SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDCTXT_COMMAND, EN_KILLFOCUS),
							(LPARAM)GetDlgItem(hDlg, IDCTXT_COMMAND));
						
						updateItem();
					}
					break;
				
				// Browse for directory
				case IDCCMD_BROWSEDIR:
					{
#ifndef OPENFILENAME_SIZE_VERSION_400
#define OPENFILENAME_SIZE_VERSION_400  sizeof(OPENFILENAME)
#endif
						
						// Get the command line file (without arguments)
						TCHAR pszDir[MAX_PATH];
						GetDlgItemText(hDlg, IDCTXT_DIRECTORY, pszDir, nbArray(pszDir));
						
						if (!BrowseForFolder(hDlg, pszDir))
							break;
						
						// Update internal structures
						SetDlgItemText(hDlg, IDCTXT_DIRECTORY, pszDir);
						SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDCTXT_DIRECTORY, EN_KILLFOCUS),
							(LPARAM)GetDlgItem(hDlg, IDCTXT_DIRECTORY));
						
						updateItem();
					}
					break;
				
				// Window show state for command line
				case IDCCBO_SHOW:
					if (HIWORD(wParam) == CBN_SELCHANGE) {
						s_psh->m_nShow =
							s_aiShow[SendDlgItemMessage(hDlg, IDCCBO_SHOW, CB_GETCURSEL, 0,0)];
						updateItem();
					}
					break;
			}
			
			
			// Notification (from list)
			case WM_NOTIFY:
				if (wParam == IDCLST) {
					NMHDR *const pNMHDR = (NMHDR*)lParam;
					switch (pNMHDR->code) {
						
						case LVN_ITEMACTIVATE:
							PostMessage(hDlg, WM_COMMAND, IDCCMD_EDIT, 0);
							break;
						
						case LVN_GETDISPINFO:
							{
								LVITEM &lvi = ((NMLVDISPINFO*)lParam)->item;
								if (lvi.mask & LVIF_TEXT) {
									Shortcut *const psh = (Shortcut*)lvi.lParam;
									switch (lvi.iSubItem) {
										
										case 0:
											if (lvi.cchTextMax >= bufHotKey)
												psh->getKeyName(lvi.pszText);
											else
												*lvi.pszText = 0;
											break;
										
										case 1:
											{
												static const TCHAR s_acCond[] = { _T('\0'), _T('+'), _T('-') };
												
												LPTSTR psz = lvi.pszText;
												for (int cond = 0; cond < 3; cond++)
													if (psh->m_aCond[cond]) {
														*psz++ = s_acCond[psh->m_aCond[cond]];
														*psz++ = s_sCondKeys.get()[cond];
													}
												*psz = _T('\0');
											}
											break;
										
										case 2:
											lstrcpyn(lvi.pszText, s_asToken[(psh->m_bCommand) ? tokCommand : tokText].get(),
												lvi.cchTextMax);
											break;
										
										case 3:
											lstrcpyn(lvi.pszText, (psh->m_bCommand) ? psh->m_sCommand : psh->m_sText,
												lvi.cchTextMax);
											break;
										
										case 4:
											lstrcpyn(lvi.pszText, psh->m_sDescription, lvi.cchTextMax);
											break;
									}
								}
							}
							break;
						
						case LVN_ITEMCHANGED:
							updateList(hDlg);
							break;
					}
					return 0;
					
				}else if (wParam == 0 && ((NMHDR*)lParam)->code == HDN_ENDTRACK) {
					// Get columns size
					LVCOLUMN lvc;
					lvc.mask = LVCF_WIDTH;
					RECT rcList;
					GetClientRect(e_hlst, &rcList);
					for (lvc.iSubItem = 0; lvc.iSubItem < nbColSize; lvc.iSubItem++) {
						ListView_GetColumn(e_hlst, lvc.iSubItem, &lvc);
						s_acxCol[lvc.iSubItem] = lvc.cx * 100 / rcList.right;
					}
				}
				break;
			
			
			case WM_GETMINMAXINFO:
				((MINMAXINFO*)lParam)->ptMinTrackSize = (POINT&)sizeMinimum;
				break;
			
			case WM_SIZE:
				{
					RECT rcClientNew;
					GetClientRect(hDlg, &rcClientNew);
					int dx = rcClientNew.right  - rcClientOrig.right;
					int dy = rcClientNew.bottom - rcClientOrig.bottom;
					
					for (int i = 0; i < nbArray(aSizePos); i++)
						aSizePos[i].updateGui(dx, dy);
					
					// Resize columns
					LVCOLUMN lvc;
					lvc.mask = LVCF_WIDTH;
					RECT rcList;
					GetClientRect(e_hlst, &rcList);
					int cxRemain = rcList.right;
					for (lvc.iSubItem = 0; lvc.iSubItem < colCount; lvc.iSubItem++) {
						lvc.cx = (lvc.iSubItem < nbColSize)
							? s_acxCol[lvc.iSubItem] * rcList.right / 100
							: cxRemain;
						cxRemain -= lvc.cx;
						ListView_SetColumn(e_hlst, lvc.iSubItem, &lvc);
					}
					
					RedrawWindow(hDlg, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_NOFRAME);
					
					getWindowSize(hDlg, s_sizeMainDialog);
				}
				break;
	}
	
	return FALSE;
}


// Language selection dialog box
BOOL CALLBACK prcLanguage(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
	const HWND hlst = GetDlgItem(hDlg, IDCLST_LANGUAGE);
	
	switch (uMsg) {
		
		case WM_INITDIALOG:
			{
				s_hdlgModal = hDlg;
				for (int lang = 0; lang < langCount; lang++)
					ListBox_AddString(hlst, s_sLanguageName.get(lang));
				ListBox_SetCurSel(hlst, e_lang);
			}
			return TRUE;
		
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDCLST_LANGUAGE:
					if (HIWORD(wParam) != LBN_DBLCLK)
						break;
				// Fall-through
				case IDOK:
					setResourceLanguage(ListBox_GetCurSel(hlst));
				case IDCANCEL:
					s_hdlgModal = NULL;
					EndDialog(hDlg, LOWORD(wParam));
					break;
			}
			break;
	}
	
	return FALSE;
}



// "About" dialog box
BOOL CALLBACK prcAbout(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		
		case WM_INITDIALOG:
			initializeWebLink(hDlg, IDCLBL_WEBSITE,        _T("http://utilfr42.free.fr/"));
			initializeWebLink(hDlg, IDCLBL_EMAIL,          _T("mailto:guillaume@ryder.fr"));
			initializeWebLink(hDlg, IDCLBL_GERMAN_WEBSITE, _T("http://www.verdrahtet.net/"));
			initializeWebLink(hDlg, IDCLBL_GERMAN_EMAIL,   _T("mailto:info@verdrahtet.net"));
			initializeWebLink(hDlg, IDCLBL_DONATE, pszDonateURL);
			return TRUE;
		
		case WM_CTLCOLORSTATIC:
			{
				int id = GetDlgCtrlID((HWND)lParam);
				if (id == IDCLBL_EMAIL        || id == IDCLBL_WEBSITE ||
				    id == IDCLBL_GERMAN_EMAIL || id == IDCLBL_GERMAN_WEBSITE) {
					HFONT hFont = (HFONT)GetWindowLong(hDlg, DWL_USER);
					if (!hFont) {
						LOGFONT lf;
						GetObject((HFONT)SendMessage((HWND)lParam, WM_GETFONT, 0,0), sizeof(lf), &lf);
						lf.lfUnderline = TRUE;
						hFont = CreateFontIndirect(&lf);
						SetWindowLong(hDlg, DWL_USER, (LONG)hFont);
					}
					SelectFont((HDC)wParam, hFont);
					SetTextColor((HDC)wParam, RGB(0,0,255));
					SetBkMode((HDC)wParam, TRANSPARENT);
					return (BOOL)GetSysColorBrush(COLOR_BTNFACE);
				}
			}
			break;
		
		case WM_COMMAND:
			DeleteFont((HFONT)GetWindowLong(hDlg, DWL_USER));
			EndDialog(hDlg, IDOK);
			break;
	}
	
	return FALSE;
}


// Redraw the selected item
void updateItem()
{
	const int iItem = ListView_GetNextItem(e_hlst, -1, LVNI_SELECTED);
	ListView_RedrawItems(e_hlst, iItem,iItem);
}



// Update the dialog box to reflect the current shortcut state
void updateList(HWND hDlg)
{
	s_bCanEvent = false;
	
	Shortcut *const psh = getSelShortcut();
	const bool bSel = ToBool(psh);
	
	static HICON s_hIcon = NULL;
	DestroyIcon(s_hIcon);
	s_hIcon = NULL;
	
	int nShow = -1;
	if (bSel) {
		// Fill shortcut controls
		
		s_psh = psh;
		CheckRadioButton(hDlg, IDCOPT_TEXT, IDCOPT_COMMAND,
			(s_psh->m_bCommand) ? IDCOPT_COMMAND : IDCOPT_TEXT);
		
		SetDlgItemText(hDlg, IDCTXT_TEXT,        s_psh->m_sText);
		SetDlgItemText(hDlg, IDCTXT_COMMAND,     s_psh->m_sCommand);
		SetDlgItemText(hDlg, IDCTXT_DIRECTORY,   s_psh->m_sDirectory);
		SetDlgItemText(hDlg, IDCTXT_DESCRIPTION, s_psh->m_sDescription);
		for (nShow = 0; nShow < nbArray(s_aiShow); nShow++)
			if (s_aiShow[nShow] == s_psh->m_nShow)
				break;
		
		// Get the icon
		TCHAR pszFile[MAX_PATH], pszExecutable[MAX_PATH];
		StrCpyN(pszFile, s_psh->m_sCommand, nbArray(pszFile));
		PathRemoveArgs(pszFile);
		findFullPath(pszFile, pszExecutable);
		
		SHFILEINFO shfi;
		if (*pszExecutable && SHGetFileInfo(pszExecutable, 0, &shfi, sizeof(shfi), SHGFI_ICON))
			s_hIcon = shfi.hIcon;
	}
	SendDlgItemMessage(hDlg, IDCCBO_SHOW, CB_SETCURSEL, (WPARAM)nShow, 0);
	
	
	// Enable or disable shortcut controls and Delete button
	
	const bool bCommand = ToBool(IsDlgButtonChecked(hDlg, IDCOPT_COMMAND));
	static const UINT s_aid[] =
	{
		IDCCMD_DELETE, IDCCMD_EDIT, IDCFRA_TEXT, IDCFRA_COMMAND,
		IDCOPT_TEXT, IDCOPT_COMMAND, IDCLBL_DESCRIPTION, IDCTXT_DESCRIPTION,
		IDCTXT_TEXT,
		IDCTXT_COMMAND, IDCCMD_BROWSE, IDCLBL_DIRECTORY, IDCTXT_DIRECTORY, IDCCMD_BROWSEDIR, IDCLBL_SHOW, IDCCBO_SHOW,
	};
	const bool abEnabled[] =
	{
		true, true, true, true,
		true, true, true, true,
		!bCommand,
		bCommand, bCommand, bCommand, bCommand, bCommand, bCommand, bCommand,
	};
	for (int i = 0; i < nbArray(s_aid); i++)
		EnableWindow(GetDlgItem(hDlg, s_aid[i]), bSel && abEnabled[i]);
	
	
	// Icon
	SendDlgItemMessage(hDlg, IDCLBL_ICON, STM_SETICON, (WPARAM)s_hIcon, 0);
	
	s_bCanEvent = true;
}


// Insert a new item in the shortcut list
// Return its ID
int addItem(Shortcut* psh)
{
	LVITEM lvi;
	lvi.iItem    = 0;
	lvi.iSubItem = 0;
	lvi.pszText  = LPSTR_TEXTCALLBACK;
	lvi.mask     = LVIF_TEXT | LVIF_PARAM;
	lvi.lParam   = (LPARAM)psh;
	lvi.iItem    = ListView_InsertItem(e_hlst, &lvi);
	lvi.mask     = LVIF_TEXT;
	for (lvi.iSubItem = 1; lvi.iSubItem < nbArray(s_acxCol); lvi.iSubItem++)
		ListView_SetItem(e_hlst, &lvi);
	
	return lvi.iItem;
}



// Handle the tray icon
void trayIconAction(DWORD dwMessage)
{
	NOTIFYICONDATA nid;
	ZeroMemory(&nid, sizeof(nid));
	nid.cbSize           = sizeof(NOTIFYICONDATA);
	nid.hWnd             = e_hwndInvisible;
	nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = msgClavierNotifyIcon;
	if (dwMessage != NIM_DELETE) {
		nid.hIcon         = (HICON)LoadImage(e_hInst, MAKEINTRESOURCE(IDI_APP), IMAGE_ICON, 16,16, 0);
		lstrcpy(nid.szTip, pszApp);
	}
	Shell_NotifyIcon(dwMessage, &nid);
}


// Open the key for Clavier+ launching at Windows startup
HKEY openAutoStartKey(LPTSTR pszPath)
{
	if (!GetModuleFileName(NULL, pszPath, MAX_PATH))
		*pszPath = 0;
	
	HKEY hKey;
	if (ERROR_SUCCESS != RegOpenKey(HKEY_CURRENT_USER,
	    _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), &hKey))
		hKey = NULL;
	
	return hKey;
}



LPCTSTR getToken(int tok)
{
	return s_asToken[tok].get();
}


// Return a tok* or tokNotFound if not found
int findToken(LPCTSTR pszToken)
{
	for (int i = 0; i < nbArray(s_asToken); i++)
		for (int lang = 0; lang < langCount; lang++)
			if (!lstrcmpi(pszToken, s_asToken[i].get(lang)))
				return i;
	
	return tokNotFound;
}


// Find a shortcut
Shortcut* findShortcut(BYTE vk, WORD vkFlags, const int aiCondState[])
{
	for (Shortcut *psh = e_pshFirst; psh; psh = psh->m_pNext)
		if (psh->match(vk, vkFlags, aiCondState))
			return psh;
	
	return NULL;
}

// Get the selected shortcut
Shortcut* getSelShortcut()
{
	LVITEM lvi;
	lvi.iItem = ListView_GetNextItem(e_hlst, -1, LVNI_SELECTED);
	if (lvi.iItem < 0)
		return NULL;
	lvi.mask = LVIF_PARAM;
	ListView_GetItem(e_hlst, &lvi);
	return (Shortcut*)lvi.lParam;
}



//------------------------------------------------------------------------
// Shortcut
//------------------------------------------------------------------------

// Constructor
Shortcut::Shortcut(const Keystroke& ks) : Keystroke(ks)
{
	m_pNext    = NULL;
	m_bCommand = false;
	m_nShow    = SW_NORMAL;
}

const LPCTSTR pszSeparator  = _T("-\r\n");

void Shortcut::save(HANDLE hf)
{
	TCHAR pszHotKey[bufHotKey], pszCode[bufCode];
	getKeyName(pszHotKey);
	wsprintf(pszCode, _T("%lu"), ((DWORD)m_vk) | (((DWORD)m_vkFlags) << 8));
	
	struct LINE
	{
		int     tokKey;
		LPCTSTR pszValue;
	};
	LINE aLine[6 + condTypeCount];
	aLine[0].tokKey   = tokShortcut;
	aLine[0].pszValue = pszHotKey;
	aLine[1].tokKey   = tokCode;
	aLine[1].pszValue = pszCode;
	int nbLine = 2;
	for (int i = 0; i < condTypeCount; i++)
		if (m_aCond[i] != condIgnore) {
			aLine[nbLine].tokKey   = tokConditionCapsLock + i;
			aLine[nbLine].pszValue = getToken(m_aCond[i] - 1 + tokConditionYes);
			nbLine++;
		}
	
	aLine[nbLine].tokKey   = tokDescription;
	aLine[nbLine].pszValue = m_sDescription;
	nbLine++;
	
	String sText;
	
	if (m_bCommand) {
		// Command
		
		aLine[nbLine].tokKey   = tokCommand;
		aLine[nbLine].pszValue = m_sCommand;
		nbLine++;
		
		aLine[nbLine].tokKey   = tokDirectory;
		aLine[nbLine].pszValue = m_sDirectory;
		nbLine++;
		
		aLine[nbLine].tokKey   = tokWindow;
		aLine[nbLine].pszValue = _T("");
		for (int i = 0; i < nbArray(s_aiShow); i++) {
			if (m_nShow == s_aiShow[i]) {
				aLine[nbLine].pszValue = getToken(tokShowNormal + i);
				break;
			}
		}
		nbLine++;
		
	}else{
		// Text
		
		// Copy the text, replace "\r\n" by "\r\n>"
		// to handle multiple lines text
		for (const TCHAR *pcFrom = m_sText; *pcFrom; pcFrom++) {
			sText += *pcFrom;
			if (pcFrom[0] == _T('\r') && pcFrom[1] == _T('\n')) {
				sText += _T("\n>");
				pcFrom++;
			}
		}
		
		aLine[nbLine].tokKey   = tokText;
		aLine[nbLine].pszValue = sText;
		nbLine++;
	}
	
	
	// Write all
	for (int i = 0; i < nbLine; i++) {
		writeFile(hf, getToken(aLine[i].tokKey));
		writeFile(hf, _T("="));
		writeFile(hf, aLine[i].pszValue);
		writeFile(hf, _T("\r\n"));
	}
	writeFile(hf, pszSeparator);
}


bool Shortcut::load(LPTSTR& rpszCurrent)
{
	// Read
	
	m_vk      = 0;
	m_vkFlags = 0;
	for (;;) {
		
		// Read the line
		LPTSTR pszLine = rpszCurrent;
		if (!pszLine)
			break;
		LPTSTR pszLineEnd = pszLine;
		while (*pszLineEnd && *pszLineEnd != _T('\n') && *pszLineEnd != _T('\r'))
			pszLineEnd++;
		if (*pszLineEnd) {
			*pszLineEnd = _T('\0');
			rpszCurrent = pszLineEnd + 1;
		}else
			rpszCurrent = NULL;
		
		int len = (int)(pszLineEnd - pszLine);
		
		// If end of shortcut, stop
		if (*pszLine == pszSeparator[0])
			break;
		
		// If the line is empty, ignore it
		if (!len)
			continue;
		
		// If next line of text, get it
		if (*pszLine == _T('>') && !m_bCommand && *m_sText) {
			m_sText += _T("\r\n");
			m_sText += pszLine + 1;
			continue;
		}
		
		// Get the key name
		TCHAR *pcSep = pszLine;
		while (*pcSep && *pcSep != _T(' ') && *pcSep != _T('='))
			pcSep++;
		const TCHAR cOldSel = *pcSep;
		*pcSep = 0;
		
		// Identify the key
		const int tokKey = findToken(pszLine);
		if (tokKey == tokNotFound)
			continue;
		
		// Get the value
		*pcSep = cOldSel;
		while (*pcSep && *pcSep != _T('='))
			pcSep++;
		if (*pcSep)
			pcSep++;
		len -= (pcSep - pszLine);
		switch (tokKey) {
			
			// Language
			case tokLanguage:
				{
					for (int lang = 0; lang < langCount; lang++)
						if (!lstrcmpi(pcSep, s_sLanguageName.get(lang)))
							setResourceLanguage(lang);
				}
				break;
			
			// Main window size
			case tokSize:
				while (*pcSep == _T(' '))
					pcSep++;
				s_sizeMainDialog.cx = StrToInt(pcSep);
				while (*pcSep && *pcSep != _T(','))
					pcSep++;
				if (*pcSep) {
					pcSep++;
					while (*pcSep == _T(' '))
						pcSep++;
				}
				s_sizeMainDialog.cy = StrToInt(pcSep);
				break;
			
			// Main window columns width
			case tokColumns:
				for (int i = 0; i < colCount - 1; i++) {
					int cx = StrToInt(pcSep);
					while (*pcSep == _T(' '))
						pcSep++;
					while (*pcSep && *pcSep != _T(','))
						pcSep++;
					if (!*pcSep)
						break;
					pcSep++;
					while (*pcSep == _T(' '))
						pcSep++;
					if (!*pcSep)
						break;
					if (cx >= 0)
						s_acxCol[i] = cx;
				}
				break;
			
			// Shortcut
			case tokShortcut:
				Keystroke::serialize(pcSep);
				break;
			
			// Code
			case tokCode:
				if (!m_vk) {
					const DWORD dwCode = (DWORD)StrToInt(pcSep);
					if (dwCode) {
						m_vk = (BYTE)(dwCode & 0xFF);
						m_vkFlags = (WORD)(dwCode >> 8);
					}
				}
				break;
			
			// Description
			case tokDescription:
				m_sDescription = pcSep;
				break;
			
			// Text or command
			case tokText:
			case tokCommand:
				m_bCommand = (tokKey == tokCommand);
				((m_bCommand) ? m_sCommand : m_sText) = pcSep;
				break;
			
			// Directory
			case tokDirectory:
				m_sDirectory = pcSep;
				break;
			
			// Window
			case tokWindow:
				{
					const int iShow = findToken(pcSep) - tokShowNormal;
					if (0 <= iShow && iShow <= nbArray(s_aiShow))
						m_nShow = s_aiShow[iShow];
				}
				break;
			
			// Condition
			case tokConditionCapsLock:
			case tokConditionNumLock:
			case tokConditionScrollLock:
				{
					int cond = findToken(pcSep);
					if (tokConditionYes <= cond && cond <= tokConditionNo)
						m_aCond[tokKey - tokConditionCapsLock] = cond - tokConditionYes + condYes;
				}
				break;
		}
	}
	
	// - Valid shortcut
	// - Not twice the same shortcut
	return (m_vk && testCanBeAdded());
}


// Add shortcut tabular representation to the given string
// Return the new end of the string
void Shortcut::appendItemToString(String& rs) const
{
	TCHAR pszHotKey[bufHotKey];
	getKeyName(pszHotKey);
	rs += pszHotKey;
	rs += _T('\t');
	
	rs += getToken((m_bCommand) ? tokCommand : tokText);
	rs += _T('\t');
	
	rs += (m_bCommand) ? m_sCommand : m_sText;
	rs += _T('\t');
	
	rs += m_sDescription;
	rs += _T("\r\n");
}



// Compare two shortcuts by shortcut name
int CALLBACK Shortcut::compare(const Shortcut* psh1, const Shortcut* psh2, LPARAM)
{
	TCHAR pszHotKey1[bufHotKey], pszHotKey2[bufHotKey];
	psh1->getKeyName(pszHotKey1);
	psh2->getKeyName(pszHotKey2);
	int diff = lstrcmpi(pszHotKey1, pszHotKey2);
	for (int i = 0; !diff && i < condTypeCount; i++)
		diff = psh1->m_aCond[i] - psh2->m_aCond[i];
	
	return diff;
}


void Shortcut::appendMenuItem(HMENU hMenu, UINT id) const
{
	TCHAR pszKeyName[bufHotKey];
	getKeyName(pszKeyName, false);
	
	TCHAR psz[1024];
	wsprintf(psz, _T("%s\t%s"), pszKeyName, m_sDescription);
	
	AppendMenu(hMenu, (id > idFirstShortcut && ((id - idFirstShortcut) % nbRowMenu) == 0)
		? MF_STRING | MF_MENUBARBREAK : MF_STRING, id, psz);
}


// Return true if we should reset the delay,
// false otherwise.
bool Shortcut::execute() const
{
	if (m_bCommand) {
		// Execute a command line
		
		THREAD_SHELLEXECUTE *const pData = new THREAD_SHELLEXECUTE(m_sCommand, m_sDirectory, m_nShow);
		DWORD dwThreadId;
		CreateThread(NULL, 0, threadShellExecute, pData, 0, &dwThreadId);
		return false;
		
	}else{
		// Text
		
		// Typing simulation requires the application has the keyboard focus
		HWND hwndFocus;
		Keystroke::catchKeyboardFocus(hwndFocus);
		
		// Send the text to the window
		bool bBackslash = false;
		LPCTSTR pszText = m_sText;
		for (int i = 0; pszText[i]; i++) {
			const UTCHAR c = (UTCHAR)pszText[i];
			if (c == _T('\n'))
				continue;
			
			if (!bBackslash && c == _T('\\')) {
				bBackslash = true;
				continue;
			}
			
			if (bBackslash || c != _T('[')) {
				bBackslash = false;
				PostMessage(hwndFocus, WM_CHAR, c, 0);
			}else{
				// Inline shortcut, or inline command line execution
				
				// Go to the end of the shortcut, and serialize it
				const LPCTSTR pcStart = pszText + i + 1;
				const TCHAR *pcEnd = pcStart;
				while (*pcEnd && (pcEnd[-1] == _T('\\' || pcEnd[0] != _T(']'))))
					pcEnd++;
				String sInside(pcStart, pcEnd - pcStart);
				
				if (sInside.isEmpty()) {
					// Nothing: catch the focus
					
					Keystroke::catchKeyboardFocus(hwndFocus);
					Sleep(100);
					
				}else if (*pcStart == _T('[') && *pcEnd == _T(']')) {
					// Double brackets: [[command line]]
					
					shellExecuteCmdLine((LPCTSTR)sInside + 1, NULL, SW_SHOWDEFAULT);
					
				}else{
					// Simple brackets: [keystroke]
					Keystroke ks;
					ks.serialize(sInside.get());
					const bool bWasRegistered = ks.unregisterHotKey();
					ks.simulateTyping();
					if (bWasRegistered)
						ks.registerHotKey();
				}
				
				i = pcEnd - pszText;
			}
		}
		
		return true;
	}
}


void copyShortcutsListToClipboard()
{
	if (!OpenClipboard(NULL))
		return;
	EmptyClipboard();
	
	// Get the text
	String s;
	for (const Shortcut *psh = e_pshFirst; psh; psh = psh->m_pNext)
		psh->appendItemToString(s);
	
	// Allocate and fill shared buffer
	const HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (s.getLength() + 1) * sizeof(TCHAR));
	LPTSTR psz = (LPTSTR)GlobalLock(hMem);
	lstrcpy(psz, s);
	GlobalUnlock(hMem);
	
	// Copy buffer to clipboard
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
}



// Get the INI file path
// pszIniFile should be a MAX_PATH length buffer
void iniGetPath(LPTSTR pszIniFile)
{
	GetModuleFileName(e_hInst, pszIniFile, MAX_PATH);
	CharLower(pszIniFile);
	PathRenameExtension(pszIniFile, _T(".ini"));
}


void iniSave()
{
	TCHAR pszIniFile[MAX_PATH];
	iniGetPath(pszIniFile);
	const HANDLE hf = CreateFile(pszIniFile,
		GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hf == INVALID_HANDLE_VALUE) {
		messageBox(NULL, ERR_SAVING_INI);
		return;
	}
	
	writeFile(hf, getToken(tokLanguage));
	writeFile(hf, _T("="));
	writeFile(hf, s_sLanguageName.get());
	writeFile(hf, _T("\r\n"));
	
	TCHAR psz[1024];
	wsprintf(psz, _T("%s=%d,%d\r\n"),
		getToken(tokSize), s_sizeMainDialog.cx, s_sizeMainDialog.cy);
	writeFile(hf, psz);
	
	writeFile(hf, getToken(tokColumns));
	for (int i = 0; i < nbColSize; i++) {
		wsprintf(psz, (i == 0) ? _T("=%d") : _T(",%d"), s_acxCol[i]);
		writeFile(hf, psz);
	}
	writeFile(hf, _T("\r\n"));
	
	for (Shortcut *psh = e_pshFirst; psh; psh = psh->m_pNext)
		psh->save(hf);
	
	CloseHandle(hf);
}


void iniLoad()
{
	for (int i = 0; i < colCount; i++)
		s_acxCol[i] = s_acxColOrig[i];
	
	TCHAR pszIniFile[MAX_PATH];
	iniGetPath(pszIniFile);
	const HANDLE hf = CreateFile(pszIniFile,
		GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hf == INVALID_HANDLE_VALUE) {
		if (GetLastError() != ERROR_FILE_NOT_FOUND) {
Error:
			messageBox(NULL, ERR_LOADING_INI);
		}
		return;
	}
	
	DWORD size = GetFileSize(hf, NULL);
	if (size == INVALID_FILE_SIZE)
		goto Error;
	
	BYTE *const pbBuffer = new BYTE[size + 2];
	DWORD lenRead;
	const bool bOK = ReadFile(hf, pbBuffer, size, &lenRead, NULL) && lenRead == size;
	CloseHandle(hf);
	
	if (!bOK) {
		delete [] pbBuffer;
		goto Error;
	}
	
	pbBuffer[size] = pbBuffer[size + 1] = 0;
	LPTSTR pszCurrent;
	if (IsTextUnicode(pbBuffer, size, NULL)) {
#ifdef UNICODE
		pszCurrent = (LPTSTR)pbBuffer;
#else
		LPWSTR wsz = (LPWSTR)pbBuffer;
		const int buf = lstrlenW(wsz) + 1;
		pszCurrent = new TCHAR[buf];
		WideCharToMultiByte(CP_ACP, 0, wsz, -1, pszCurrent, buf, NULL, NULL);
#endif
	}else{
#ifdef UNICODE
		LPSTR wsz = (LPSTR)pbBuffer;
		const int buf = lstrlenA(psz) + 1;
		pszCurrent = new TCHAR[buf];
		MultiByteToWideChar(CP_ACP, 0, wsz, -1, pszCurrent, buf);
#else
		pszCurrent = (LPTSTR)pbBuffer;
#endif
	}
	
	Keystroke ks;
	do{
		Shortcut *const psh = new Shortcut(ks);
		if (psh->load(pszCurrent)) {
			psh->m_pNext = e_pshFirst;
			e_pshFirst = psh;
			psh->registerHotKey();
		}else
			delete psh;
	}while (pszCurrent);
	
	delete [] pbBuffer;
}
