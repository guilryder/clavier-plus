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
#include "App.h"


const LPCTSTR pszValueAutoStart = pszApp;

const LPCTSTR pszDonateURL = _T("https://www.paypal.com/cgi-bin/webscr?cmd=_xclick&business=gryder%40club%2dinternet%2efr&item_name=UtilFr%20%2d%20Clavier%2b&no_shipping=2&no_note=1&tax=0&currency_code=EUR&bn=PP%2dDonationsBF&charset=UTF%2d8");
const LPCTSTR pszHelpURL = _T("http://utilfr42.free.fr/util/Clavier");

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


static TranslatedString s_asToken[tokNotFound];
static TranslatedString s_sCondKeys;


static HWND s_hdlgModal = NULL;
       HWND s_hdlgMain;
       HWND e_hlst = NULL;       // Shortcut list
static Shortcut *s_psh = NULL;   // Current edited shortcut
       Shortcut *e_pshFirst;     // Shortcut list

// Process GUI events to update shortcuts data from the dialog box controls
static bool s_bProcessGuiEvents;

// Last known foreground window
// Used by shortcuts menu, to send keys to another window
// than the traybar or Clavier+ invisible window.
static HWND s_hwndLastForeground = NULL;


static LRESULT CALLBACK prcInvisible  (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL    CALLBACK prcMain       (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL    CALLBACK prcCmdSettings(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL    CALLBACK prcLanguage   (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL    CALLBACK prcAbout      (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK prcProgramsTarget(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static WNDPROC s_prcProgramsTarget;

// ID of the next menu item to add to the "Add" sub-menus (programs and favorites)
static UINT s_idAddMenuId;

static bool browseForCommandLine(HWND hwndParent, LPTSTR pszFile, bool bForceExist);
static bool browseForCommandLine(HWND hDlg);
static void onMainCommand(UINT id, WORD wNotify, HWND hWnd);


static void trayIconAction(DWORD dwMessage);

static void updateItem();
static void updateList();
static int  addItem(Shortcut* psh, bool bSelected);
static void onItemUpdated(int mskColumns);

static Shortcut* findShortcut(BYTE vk, WORD vkFlags, const int aiCondState[], LPCTSTR pszProgram);
static Shortcut* getSelShortcut();

static DWORD WINAPI threadGetSmallIcons(void* agfi);
static DWORD WINAPI threadGetFileIcon(void* pgfi);

static void fillSpecialCharsMenu(HMENU hMenu, int pos, UINT idFirst);
static void escapeString(LPCTSTR, String& rs);
static void appendText(LPCTSTR pszText);



#ifdef _DEBUG
static void WinMainCRTStartup();

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	WinMainCRTStartup();
	return 0;
}
#endif // _DEBUG


// Entry point
void WinMainCRTStartup()
{
	msgTaskbarCreated = RegisterWindowMessage(_T("TaskbarCreated"));
	msgClavierNotifyIcon = RegisterWindowMessage(_T("RyderClavierOptions"));
	
#ifndef _DEBUG
	// Avoid multiple launching
	// Show the main dialog box if already launched
	CreateMutex(NULL, TRUE, pszApp);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		SendMessage(HWND_BROADCAST, msgClavierNotifyIcon, 0, WM_LBUTTONDOWN);
		ExitProcess(0);
		return;
	}
#endif
	
	CoInitialize(NULL);
	
	e_hInst = (HINSTANCE)GetModuleHandle(NULL);
	e_hHeap = GetProcessHeap();
	
	for (int lang = 0; lang < langCount; lang++) {
		setLanguage(lang);
		
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
		
		s_sCondKeys.load(IDS_CONDITION_KEYS);
	}
	
	setLanguage(getDefaultLanguage());
	
	
	// Load the INI file
	iniLoad();
	HeapCompact(e_hHeap, 0);
	
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
			
			HWND hwndFocus;
			Keystroke::catchKeyboardFocus(hwndFocus);
			
			static const int avkCond[] =
			{
				VK_CAPITAL, VK_NUMLOCK, VK_SCROLL,
			};
			int aCondState[condTypeCount];
			for (int i = 0; i < condTypeCount; i++)
				aCondState[i] = (GetKeyState(avkCond[i]) & 0x01) ? condYes : condNo;
			
			TCHAR pszProcess[MAX_PATH];
			if (!getWindowExecutable(hwndFocus, pszProcess))
				*pszProcess = _T('\0');
			
			Shortcut *const psh = findShortcut(
				(BYTE)HIWORD(msg.lParam), LOWORD(msg.lParam), aCondState,
				(*pszProcess) ? pszProcess : NULL);
			
			if (psh) {
				if (psh->execute())
					timeMinimum = GetTickCount();
			}else{
				Keystroke ks;
				ks.m_vk      = (BYTE)HIWORD(msg.lParam);
				ks.m_vkFlags = LOWORD(msg.lParam);
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
	
	CoUninitialize();
#ifndef _DEBUG
	ExitProcess(0);
#endif
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
				HeapCompact(e_hHeap, 0);
				switch (dialogBox(IDD_MAIN, NULL, prcMain)) {
					case IDCCMD_LANGUAGE:
						dialogBox(IDD_LANGUAGE, NULL, prcLanguage);
						break;
					case IDCCMD_QUIT:
						goto Destroy;
					default:
						HeapCompact(e_hHeap, 0);
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
	
	{ IDCOPT_TEXT,      sizeposTop },
	{ IDCFRA_TEXT,      sizeposTop | sizeposWidthHalf },
	{ IDCTXT_TEXT,      sizeposTop | sizeposWidthHalf },
	{ IDCCMD_TEXT_MENU, sizeposTop | sizeposLeftHalf },
	
	{ IDCOPT_COMMAND,     sizeposTop | sizeposLeftHalf },
	{ IDCFRA_COMMAND,     sizeposTop | sizeposLeftHalf | sizeposWidthHalf },
	{ IDCTXT_COMMAND,     sizeposTop | sizeposLeftHalf | sizeposWidthHalf },
	{ IDCCMD_COMMAND,     sizeposTop | sizeposLeft },
	{ IDCLBL_ICON,        sizeposTop | sizeposLeftHalf },
	{ IDCCMD_CMDSETTINGS, sizeposTop | sizeposLeftHalf },
	{ IDCCMD_TEST,        sizeposTop | sizeposLeftHalf },
	
	{ IDCLBL_PROGRAMS, sizeposTop },
	{ IDCTXT_PROGRAMS, sizeposTop | sizeposWidth },
	{ IDCIMG_PROGRAMS, sizeposTop | sizeposLeft },
	
	{ IDCLBL_DESCRIPTION, sizeposTop },
	{ IDCTXT_DESCRIPTION, sizeposTop | sizeposWidth },
	
	{ IDCLBL_DONATE, sizeposTop },
	
	{ IDCCMD_HELP,     sizeposTop | sizeposLeft },
	{ IDCCMD_COPYLIST, sizeposTop | sizeposLeft },
	{ IDCCMD_LANGUAGE, sizeposTop | sizeposLeft },
	{ IDCCMD_ABOUT,    sizeposTop | sizeposLeft },
	{ IDCCMD_QUIT,     sizeposTop | sizeposLeft },
	{ IDCANCEL,        sizeposTop | sizeposLeft },
};


static void createAndAddShortcut(LPCTSTR pszCommand = NULL);
static Shortcut* createShortcut();
static void addCreatedShortcut(Shortcut* psh);


void createAndAddShortcut(LPCTSTR pszCommand)
{
	Shortcut *const psh = createShortcut();
	if (psh) {
		if (pszCommand) {
			psh->m_bCommand = true;
			psh->m_sCommand = pszCommand;
			PathQuoteSpaces(psh->m_sCommand.getBuffer(MAX_PATH));
		}
		addCreatedShortcut(psh);
	}
}

Shortcut* createShortcut()
{
	SetFocus(e_hlst);
	Keystroke ks;
	return (AskKeystroke(s_hdlgMain, NULL, ks))
		? new Shortcut(ks) : NULL;
}

void addCreatedShortcut(Shortcut* psh)
{
	addItem(psh, true);
	onItemUpdated(~0);
	updateList();
	
	const HWND hWnd = GetDlgItem(s_hdlgMain, (psh->m_bCommand) ? IDCTXT_COMMAND : IDCTXT_TEXT);
	if (psh->m_bCommand) {
		const int len = psh->m_sCommand.getLength();
		Edit_SetSel(hWnd, len, len);
	}
	SetFocus(hWnd);
}


class FileMenuItem
{
public:
	
	FileMenuItem(bool bStartMenu, bool bIsDir, LPCTSTR pszLabel, LPCTSTR pszPath, int iconIndex)
		: m_bStartMenu(bStartMenu), m_bIsDir(bIsDir),
		m_sLabel(pszLabel), m_sPath(pszPath), m_iconIndex(iconIndex) {}
	
	bool isDir() const { return m_bIsDir; }
	void measureItem(HDC hdc, MEASUREITEMSTRUCT& mis);
	void drawItem(DRAWITEMSTRUCT& dis);
	void populate(HMENU hMenu);
	void execute();
	
private:
	
	bool   m_bStartMenu;
	bool   m_bIsDir;
	String m_sLabel;
	String m_sPath;
	int    m_iconIndex;
	
	enum{ cxLeft = 3, cxBetween = 5, cxRight = 16 };
};


static FileMenuItem* findItemAndCleanMenu(HMENU hMenu, UINT id);

static HIMAGELIST s_hSysImageList = NULL;


FileMenuItem* findItemAndCleanMenu(HMENU hMenu, UINT id)
{
	MENUITEMINFO mii;
	mii.cbSize     = sizeof(mii);
	mii.fMask      = MIIM_FTYPE | MIIM_DATA | MIIM_ID | MIIM_SUBMENU;
	
	FileMenuItem *pItemToExecute = NULL;
	for (UINT pos = 0; GetMenuItemInfo(hMenu, pos, TRUE, &mii); pos++) {
		FileMenuItem *pItem = (FileMenuItem*)mii.dwItemData;
		if (pItem) {
			if (mii.wID == id && id)
				pItemToExecute = pItem;
			else if (!(mii.fType & MFT_SEPARATOR))
				delete pItem;
		}
		if (mii.hSubMenu) {
			pItem = findItemAndCleanMenu(mii.hSubMenu, id);
			if (pItem && !pItemToExecute)
				pItemToExecute = pItem;
		}
	}
	
	return pItemToExecute;
}



#pragma warning(disable: 4701)
void FileMenuItem::populate(HMENU hMenu)
{
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fType  = MFT_OWNERDRAW;
	
	MENUITEMINFO miiSub;
	miiSub.cbSize = sizeof(miiSub);
	miiSub.fMask      = MIIM_TYPE | MIIM_DATA;
	miiSub.fType      = MFT_SEPARATOR;
	miiSub.dwTypeData = _T("");
	
	MENUITEMINFO miiParam;
	miiParam.cbSize = sizeof(mii);
	miiParam.fMask  = MIIM_DATA | MIIM_TYPE;
	miiParam.cch    = 0;
	
	UINT posFolder = 0;
	
	for (int iSource = 0; iSource < 2; iSource++) {
		TCHAR pszFolder[MAX_PATH];
		
		if (m_bStartMenu) {
			if (!getSpecialFolderPath((iSource) ? CSIDL_COMMON_STARTMENU : CSIDL_STARTMENU, pszFolder))
				continue;
			if (m_sPath.isSome())
				PathAppend(pszFolder, m_sPath);
			
		}else{
			if (iSource > 0)
				break;
			lstrcpyn(pszFolder, m_sPath, nbArray(pszFolder));
		}
		
		TCHAR pszPath[MAX_PATH];
		PathCombine(pszPath, pszFolder, _T("*"));
		WIN32_FIND_DATA wfd;
		const HANDLE hff = FindFirstFile(pszPath, &wfd);
		if (hff == INVALID_HANDLE_VALUE)
			continue;
		do{
			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
				continue;
			
			PathCombine(pszPath, pszFolder, wfd.cFileName);
			
			TCHAR pszRelativeFolder[MAX_PATH];
			LPTSTR pszItemPath;
			
			const bool bIsDir = ToBool(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
			
			// Directories: submenus
			// Files: only *.lnk files
			UINT pos;
			if (bIsDir) {
				if (wfd.cFileName[0] == _T('.') &&
					(wfd.cFileName[1] == _T('.') || !wfd.cFileName[1]))
					continue;
				
				lstrcpyn(pszRelativeFolder, m_sPath, nbArray(pszRelativeFolder));
				PathAppend(pszRelativeFolder, wfd.cFileName);
				pszItemPath = pszRelativeFolder;
				
				if (iSource) {
					const int nbItem = GetMenuItemCount(hMenu);
					for (int i = 0; i < nbItem; i++) {
						if (!GetMenuItemInfo(hMenu, i, TRUE, &mii))
							continue;
						const FileMenuItem &fmiOther = *(const FileMenuItem*)mii.dwItemData;
						if (fmiOther.isDir() && !lstrcmpi(fmiOther.m_sPath, pszRelativeFolder))
							goto Next;
					}
				}
				
				mii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_SUBMENU;
				mii.hSubMenu = CreatePopupMenu();
				pos = posFolder++;
				
			}else{
				mii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_ID;
				mii.wID   = s_idAddMenuId++;
				pos = (UINT)-1;
				PathRemoveExtension(wfd.cFileName);
				pszItemPath = pszPath;
			}
			
			// Get the small icon
			SHFILEINFO shfi;
			if (!getFileInfo(pszPath, wfd.dwFileAttributes, shfi,
			    SHGFI_ICON | SHGFI_SMALLICON | SHGFI_SYSICONINDEX))
				shfi.iIcon = -1;
			
			mii.dwItemData = (ULONG_PTR)new FileMenuItem(bIsDir, m_bStartMenu,
				wfd.cFileName, pszItemPath, shfi.iIcon);
			
			// Create the phantom sub-item used by WM_INITMENUPOPUP
			// to get the parent of the popup to initialize
			if (bIsDir) {
				miiSub.dwItemData = mii.dwItemData;
				InsertMenuItem(mii.hSubMenu, 0, TRUE, &miiSub);
			}
			
			InsertMenuItem(hMenu, pos, TRUE, &mii);
			
		Next:
			;
		}while (FindNextFile(hff, &wfd));
		FindClose(hff);
	}
	
	const int nbItemPerColumn =
		(GetSystemMetrics(SM_CYSCREEN) - 10) / GetSystemMetrics(SM_CYMENU);
	
	mii.fMask = MIIM_TYPE;
	int nbItem = GetMenuItemCount(hMenu);
	for (int i = nbItemPerColumn; i < nbItem; i += nbItemPerColumn) {
		GetMenuItemInfo(hMenu, i, TRUE, &mii);
		mii.fType |= MFT_MENUBARBREAK;
		SetMenuItemInfo(hMenu, i, TRUE, &mii);
	}
}
#pragma warning(default: 4701)


void FileMenuItem::measureItem(HDC hdc, MEASUREITEMSTRUCT& mis)
{
	SelectFont(hdc, GetStockFont(DEFAULT_GUI_FONT));
	
	RECT rc;
	DrawText(hdc, m_sLabel, -1, &rc,
		DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_CALCRECT);
	
	mis.itemWidth  = cxLeft + GetSystemMetrics(SM_CXSMICON) + cxBetween + rc.right - rc.left + cxRight;
	mis.itemHeight = rc.bottom - rc.top;
	int cyMenu = GetSystemMetrics(SM_CYMENU);
	int cyIcon = GetSystemMetrics(SM_CXSMICON);
	if (cyMenu < cyIcon)
		cyMenu = cyIcon;
	if ((int)mis.itemHeight < cyMenu)
		mis.itemHeight = cyMenu;
}


void FileMenuItem::drawItem(DRAWITEMSTRUCT& dis)
{
	int sysColorText, sysColorBack;
	if (dis.itemState & ODS_SELECTED) {
		sysColorText = COLOR_HIGHLIGHTTEXT;
		sysColorBack = COLOR_HIGHLIGHT;
	}else{
		sysColorText = COLOR_MENUTEXT;
		sysColorBack = COLOR_MENU;
	}
	
	COLORREF clTextOld = SetTextColor(dis.hDC, GetSysColor(sysColorText));
	COLORREF clBackOld = SetBkColor  (dis.hDC, GetSysColor(sysColorBack));
	ExtTextOut(dis.hDC, dis.rcItem.left, dis.rcItem.top,
		ETO_CLIPPED | ETO_OPAQUE, &dis.rcItem, "", 0, NULL);
	
	ImageList_Draw(s_hSysImageList, m_iconIndex, dis.hDC,
		dis.rcItem.left + cxLeft,
		(dis.rcItem.top + dis.rcItem.bottom - GetSystemMetrics(SM_CYSMICON)) / 2,
		ILD_TRANSPARENT);
	
	const int cxOffset = cxLeft + cxBetween + GetSystemMetrics(SM_CXSMICON);
	dis.rcItem.left += cxOffset;
	DrawText(dis.hDC, m_sLabel, -1, &dis.rcItem,
		DT_LEFT | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE);
	dis.rcItem.left -= cxOffset;
	
	SetTextColor(dis.hDC, clTextOld);
	SetBkColor  (dis.hDC, clBackOld);
}


void FileMenuItem::execute()
{
	TCHAR pszPath[MAX_PATH];
	if (!getShellLinkTarget(m_sPath, pszPath))
		lstrcpy(pszPath, m_sPath);
	
	createAndAddShortcut(pszPath);
}


bool browseForCommandLine(HWND hwndParent, LPTSTR pszFile, bool bForceExist)
{
#ifndef OPENFILENAME_SIZE_VERSION_400
#define OPENFILENAME_SIZE_VERSION_400  sizeof(OPENFILENAME)
#endif
	
	PathUnquoteSpaces(pszFile);
	
	// Ask for the file
	OPENFILENAME ofn;
	ZeroMemory(&ofn, OPENFILENAME_SIZE_VERSION_400);
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner   = hwndParent;
	ofn.lpstrFile   = pszFile;
	ofn.nMaxFile    = MAX_PATH;
	ofn.Flags       = (bForceExist) ? OFN_FILEMUSTEXIST | OFN_HIDEREADONLY : OFN_HIDEREADONLY;
	VERIF(GetOpenFileName(&ofn));
	
	PathQuoteSpaces(pszFile);
	return true;
}


bool browseForCommandLine(HWND hDlg)
{
	TCHAR pszFile[MAX_PATH];
	GetDlgItemText(hDlg, IDCTXT_COMMAND, pszFile, nbArray(pszFile));
	PathRemoveArgs(pszFile);
	
	VERIF(browseForCommandLine(hDlg, pszFile, true));
	
	SetDlgItemText(hDlg, IDCTXT_COMMAND, pszFile);
	return true;
}


static bool s_bCapturingProgram = false;
static String s_sOldPrograms;

void onMainCommand(UINT id, WORD wNotify, HWND hWnd)
{
	switch (id) {
		
		case IDCCMD_QUIT:
		case IDCANCEL:
			{
				// Get all shortcuts, check unicity
				const int nbItem = ListView_GetItemCount(e_hlst);
				Shortcut **const ash = new Shortcut*[nbItem];
				LVITEM lvi;
				lvi.mask = LVIF_PARAM;
				for (lvi.iItem = 0; lvi.iItem < nbItem; lvi.iItem++) {
					ListView_GetItem(e_hlst, &lvi);
					Shortcut *const psh = ash[lvi.iItem] = (Shortcut*)lvi.lParam;
					String *const asProgram = psh->getPrograms();
					for (int i = 0; i < lvi.iItem; i++)
						if (ash[i]->testConflict(*psh, asProgram)) {
							TCHAR pszHotKey[bufHotKey];
							psh->getKeyName(pszHotKey);
							messageBox(s_hdlgMain, ERR_SHORTCUT_DUPLICATE, MB_ICONERROR, pszHotKey);
							delete [] asProgram;
							delete [] ash;
							return;
						}
					delete [] asProgram;
				}
				
				if (id == IDCCMD_QUIT && !(GetKeyState(VK_SHIFT) & 0x8000) &&
				    IDNO == messageBox(s_hdlgMain, ASK_QUIT, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION))
					break;
				
				// Create a valid linked list from the list box
				// and register the hot keys
				e_pshFirst = NULL;
				for (int i = nbItem; --i >= 0;) {
					Shortcut *const psh = ash[i];
					psh->resetIcons();
					psh->registerHotKey();
					psh->m_pNext = e_pshFirst;
					e_pshFirst = psh;
				}
				delete [] ash;
				
				// Create/delete the auto start key
				TCHAR pszAutoStartPath[MAX_PATH];
				const HKEY hKeyAutoStart = openAutoStartKey(pszAutoStartPath);
				if (hKeyAutoStart) {
					if (IsDlgButtonChecked(s_hdlgMain, IDCCHK_AUTOSTART))
						RegSetValueEx(hKeyAutoStart, pszValueAutoStart, NULL, REG_SZ,
							(const BYTE*)pszAutoStartPath, lstrlen(pszAutoStartPath) + 1);
					else
						RegDeleteValue(hKeyAutoStart, pszValueAutoStart);
					
					RegCloseKey(hKeyAutoStart);
				}
			}
			
			iniSave();
			s_hdlgModal = NULL;
			EndDialog(s_hdlgMain, id);
			break;
		
		// Display "Add" menu
		case IDCCMD_ADD:
			{
				const HMENU hAllMenus = loadMenu(IDM_CONTEXT);
				
				HMENU hMenu = GetSubMenu(hAllMenus, 0);
				TCHAR pszFolder[MAX_PATH];
				
				s_idAddMenuId = ID_ADD_PROGRAM_FIRST;
				
				MENUITEMINFO mii;
				mii.cbSize     = sizeof(mii);
				mii.fMask      = MIIM_DATA;
				
				MENUITEMINFO miiSub;
				miiSub.cbSize     = sizeof(miiSub);
				miiSub.fMask      = MIIM_TYPE | MIIM_DATA;
				miiSub.fType      = MFT_SEPARATOR;
				miiSub.dwTypeData = _T("");
				
				// Programs
				mii.dwItemData = miiSub.dwItemData =
					(ULONG_PTR)new FileMenuItem(true, true, NULL, NULL, 0);
				SetMenuItemInfo(hMenu, 0, TRUE, &mii);
				SetMenuItemInfo(GetSubMenu(hMenu, 0), 0, TRUE, &miiSub);
				
				// Favorites
				if (getSpecialFolderPath(CSIDL_FAVORITES, pszFolder)) {
					mii.dwItemData = miiSub.dwItemData =
						(ULONG_PTR)new FileMenuItem(false, true, NULL, pszFolder, 0);
					SetMenuItemInfo(hMenu, 1, TRUE, &mii);
					SetMenuItemInfo(GetSubMenu(hMenu, 1), 0, TRUE, &miiSub);
				}
				
				fillSpecialCharsMenu(hMenu, 2, ID_ADD_SPECIALCHAR_FIRST);
				
				// Show the context menu
				RECT rc;
				GetWindowRect(hWnd, &rc);
				const UINT id = (UINT)TrackPopupMenu(hMenu,
					TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD,
					rc.left, rc.bottom, 0, s_hdlgMain, NULL);
				FileMenuItem *const pItem = findItemAndCleanMenu(hMenu, id);
				DestroyMenu(hAllMenus);
				
				if (pItem) {
					pItem->execute();
					delete pItem;
				}else if (id)
					SendMessage(s_hdlgMain, WM_COMMAND, id, 0);
			}
			break;
		
		case ID_ADD_FOLDER:
			{
				TCHAR pszFolder[MAX_PATH] = _T("");
				if (browseForFolder(s_hdlgMain, NULL, pszFolder))
					createAndAddShortcut(pszFolder);
			}
			break;
		
		case ID_ADD_TEXT:
			createAndAddShortcut();
			break;
		
		case ID_ADD_COMMAND:
			createAndAddShortcut(_T(""));
			break;
		
		case ID_ADD_WEBSITE:
			createAndAddShortcut(_T("http://"));
			break;
		
		
		case IDCCMD_DELETE:
			{
				if (!(GetKeyState(VK_SHIFT) & 0x8000) && IDNO == messageBox(
				    s_hdlgMain, ASK_DELETE, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION))
					break;
				
				SetFocus(GetDlgItem(s_hdlgMain, IDCCMD_ADD));
				SendMessage(s_hdlgMain, WM_NEXTDLGCTL, TRUE, FALSE);
				
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
				updateList();
				SetFocus(e_hlst);
			}
			break;
		
		case IDCCMD_EDIT:
			SetFocus(e_hlst);
			{
				Keystroke ks = *s_psh;
				if (AskKeystroke(s_hdlgMain, s_psh, ks)) {
					(Keystroke&)*s_psh = ks;
					updateItem();
					onItemUpdated((1 << colShortcut) | (1 << colCond));
					updateList();
				}
			}
			break;
		
		
		case ID_TEXT_SENDKEYS:
			{
				Keystroke ks;
				if (Keystroke::askSendKeys(s_hdlgMain, ks)) {
					TCHAR pszHotKey[bufHotKey];
					ks.getKeyName(pszHotKey);
					
					String s = _T("[");
					escapeString(pszHotKey, s);
					s += _T(']');
					
					appendText(s);
				}
			}
			break;
		
		case IDCCMD_HELP:
			{
				TCHAR pszPath[MAX_PATH];
				GetModuleFileName(e_hInst, pszPath, nbArray(pszPath));
				PathRemoveFileSpec(pszPath);
				
				TCHAR pszFile[MAX_PATH];
				loadStringAuto(IDS_HELP, pszFile);
				PathAppend(pszPath, pszFile);
				
				if (32 >= (DWORD)ShellExecute(NULL, NULL, pszPath, NULL, NULL, SW_SHOWDEFAULT)) {
					lstrcpy(pszPath, pszHelpURL);
					lstrcat(pszPath, pszFile);
					ShellExecute(NULL, NULL, pszPath, NULL, NULL, SW_SHOWDEFAULT);
				}
			}
			break;
		
		
		case IDCCMD_LANGUAGE:
			EndDialog(s_hdlgMain, IDCCMD_LANGUAGE);
			break;
		
		
		case IDCCMD_ABOUT:
			dialogBox(IDD_ABOUT, s_hdlgMain, prcAbout);
			break;
		
		
		case IDCCMD_COPYLIST:
			copyShortcutsListToClipboard();
			messageBox(s_hdlgMain, MSG_COPYLIST, MB_ICONINFORMATION);
			break;
		
		
		// - Type text
		// - Run command line
		case IDCOPT_TEXT:
		case IDCOPT_COMMAND:
			{
				const bool bCommand = (id == IDCOPT_COMMAND);
				s_psh->m_bCommand = bCommand;
				s_psh->resetIcons();
				updateList();
				updateItem();
				SetFocus(GetDlgItem(s_hdlgMain, (bCommand) ? IDCTXT_COMMAND : IDCTXT_TEXT));
				onItemUpdated(1 << colContents);
			}
			break;
		
		
		// Text, command, programs, description
		case IDCTXT_TEXT:
		case IDCTXT_COMMAND:
		case IDCTXT_PROGRAMS:
		case IDCTXT_DESCRIPTION:
			if (s_bProcessGuiEvents && wNotify == EN_CHANGE) {
				int iColumn = -1;
				String *ps;
				switch (id) {
					case IDCTXT_TEXT:       ps = &s_psh->m_sText;         break;
					case IDCTXT_COMMAND:    ps = &s_psh->m_sCommand;      iColumn = colContents;  break;
					case IDCTXT_PROGRAMS:   ps = &s_psh->m_sPrograms;     iColumn = colCond;  break;
					default:                ps = &s_psh->m_sDescription;  iColumn = colDescription;  break;
				}
				
				getDlgItemText(s_hdlgMain, id, *ps);
				
				if (id == IDCTXT_COMMAND) {
					s_psh->resetIcons();
					s_psh->getIcon();
				}
				
				updateItem();
				
				if (iColumn >= 0)
					onItemUpdated(1 << iColumn);
			}
			break;
		
		// Browse for command line
		case IDCCMD_COMMAND:
			if (browseForCommandLine(s_hdlgMain)) {
				SendMessage(s_hdlgMain, WM_COMMAND, MAKEWPARAM(IDCTXT_COMMAND, EN_KILLFOCUS),
					(LPARAM)GetDlgItem(s_hdlgMain, IDCTXT_COMMAND));
				updateItem();
			}
			break;
		
		case IDCCMD_CMDSETTINGS:
			if (IDOK == dialogBox(IDD_CMDSETTINGS, s_hdlgMain, prcCmdSettings)) {
				updateList();
				updateItem();
				onItemUpdated(1 << colContents);
			}
			break;
		
		case IDCCMD_TEST:
			s_psh->execute();
			break;
		
		case IDCCMD_TEXT_MENU:
			{
				const HMENU hAllMenus = loadMenu(IDM_CONTEXT);
				const HMENU hMenu = GetSubMenu(hAllMenus, 1);
				
				fillSpecialCharsMenu(hMenu, 4, ID_TEXT_SPECIALCHAR_FIRST);
				
				RECT rc;
				GetWindowRect(hWnd, &rc);
				SetFocus(GetDlgItem(s_hdlgMain, IDCTXT_TEXT));
				TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON,
					rc.left, rc.bottom, 0, s_hdlgMain, NULL);
				
				DestroyMenu(hAllMenus);
			}
			break;
		
		case ID_TEXT_BACKSLASH:
			appendText(_T("\\\\"));
			break;
		
		case ID_TEXT_BRACKET:
			appendText(_T("\\["));
			break;
		
		case ID_TEXT_WAIT:
			appendText(_T("[]"));
			break;
		
		case ID_TEXT_COMMAND:
			{
				TCHAR pszFile[MAX_PATH] = _T("");
				if (browseForCommandLine(s_hdlgMain, pszFile, false)) {
					String s = _T("[[");
					escapeString(pszFile, s);
					s += _T("]]");
					appendText(s);
				}
			}
			break;
		
		default:
			if (ID_ADD_SPECIALCHAR_FIRST <= id && id < ID_ADD_PROGRAM_FIRST) {
				Shortcut *const psh = createShortcut();
				if (psh) {
					const TCHAR pszText[] = { (TCHAR)(id - ID_ADD_SPECIALCHAR_FIRST), _T('\0') };
					psh->m_bCommand = false;
					psh->m_sText    = pszText;
					addCreatedShortcut(psh);
				}
			}else if (ID_TEXT_SPECIALCHAR_FIRST <= id && id < ID_TEXT_SPECIALCHAR_FIRST + 256) {
				const TCHAR pszText[] = { (TCHAR)(id - ID_TEXT_SPECIALCHAR_FIRST), _T('\0') };
				String s;
				escapeString(pszText, s);
				appendText(s);
			}
			break;
	}
}


// Main dialog box
BOOL CALLBACK prcMain(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static RECT rcClientOrig;
	static SIZE sizeMinimum;
	
	switch (uMsg) {
		
		// Initialization
		case WM_INITDIALOG:
			{
				s_bProcessGuiEvents = false;
				
				s_prcProgramsTarget = SubclassWindow(GetDlgItem(hDlg, IDCIMG_PROGRAMS), prcProgramsTarget);
				
				s_hdlgMain = s_hdlgModal = hDlg;
				e_hlst = GetDlgItem(hDlg, IDCLST);
				ListView_SetExtendedListViewStyle(e_hlst, LVS_EX_FULLROWSELECT);
				
				// Get the initial size
				GetWindowRect(hDlg, &rcClientOrig);
				sizeMinimum.cx = rcClientOrig.right  - rcClientOrig.left;
				sizeMinimum.cy = rcClientOrig.bottom - rcClientOrig.top;
				GetClientRect(hDlg, &rcClientOrig);
				
				// Get the initial position of controls
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
				SendDlgItemMessage(hDlg, IDCLBL_DONATE, STM_SETIMAGE, IMAGE_BITMAP,
					(LPARAM)loadBitmap(IDB_DONATE));
				
				SendDlgItemMessage(hDlg, IDCCMD_TEXT_MENU, BM_SETIMAGE, IMAGE_BITMAP,
					(LPARAM)LoadBitmap(NULL, MAKEINTRESOURCE(OBM_COMBO)));
				
				// Get the handle of the system image list for small icons
				SHFILEINFO shfi;
				s_hSysImageList = (HIMAGELIST)SHGetFileInfo(
					_T("a"), FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(shfi),
					SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_SMALLICON | SHGFI_SYSICONINDEX);
				ListView_SetImageList(e_hlst, s_hSysImageList, LVSIL_SMALL);
				
				// Add columns to the list
				TCHAR pszColumns[512];
				loadStringAuto(IDS_COLUMNS, pszColumns);
				TCHAR *pcColumn = pszColumns;
				LVCOLUMN lvc;
				lvc.mask = LVCF_TEXT | LVCF_SUBITEM;
				for (lvc.iSubItem = 0; lvc.iSubItem < colCount; lvc.iSubItem++) {
					lvc.pszText = pcColumn;
					while (*pcColumn != _T(';'))
						pcColumn++;
					*pcColumn++ = 0;
					ListView_InsertColumn(e_hlst, lvc.iSubItem, &lvc);
				}
				
				// Fill shortcuts list and unregister hot keys
				int nbShortcut = 0;
				for (Shortcut *psh = e_pshFirst; psh; psh = psh->m_pNext) {
					psh->unregisterHotKey();
					addItem(psh, (s_psh == psh));
					if (psh->m_bCommand)
						nbShortcut++;
				}
				onItemUpdated(~0);
				
				// Load small icons
				GETFILEICON **const apgfi = new GETFILEICON*[nbShortcut + 1];
				apgfi[nbShortcut] = NULL;
				for (Shortcut *psh = e_pshFirst; psh; psh = psh->m_pNext)
					if (psh->m_bCommand)
						psh->fillGetFileIcon(apgfi[--nbShortcut] = new GETFILEICON, true);
				startThread(threadGetSmallIcons, apgfi);
				
				updateList();
				
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
				
				s_bProcessGuiEvents = true;
				
				SendMessage(hDlg, WM_SETICON, FALSE,
					(LPARAM)(HICON)LoadImage(e_hInst, MAKEINTRESOURCE(IDI_APP), IMAGE_ICON, 16,16, 0));
				SendMessage(hDlg, WM_SETICON, TRUE,
					(LPARAM)LoadIcon(e_hInst, MAKEINTRESOURCE(IDI_APP)));
				
				// Restore window size from settings
				if (e_sizeMainDialog.cx < sizeMinimum.cx || e_sizeMainDialog.cy < sizeMinimum.cy)
					e_sizeMainDialog = sizeMinimum;
				else{
					const HMONITOR hMonitor = MonitorFromWindow(hDlg, MONITOR_DEFAULTTOPRIMARY);
					MONITORINFO mi;
					mi.cbSize = sizeof(mi);
					GetMonitorInfo(hMonitor, &mi);
					
					WINDOWPLACEMENT wp;
					GetWindowPlacement(hDlg, &wp);
					wp.rcNormalPosition.left   = (mi.rcWork.left + mi.rcWork.right  - e_sizeMainDialog.cx) / 2;
					wp.rcNormalPosition.top    = (mi.rcWork.top  + mi.rcWork.bottom - e_sizeMainDialog.cy) / 2;
					wp.rcNormalPosition.right  = wp.rcNormalPosition.left + e_sizeMainDialog.cx;
					wp.rcNormalPosition.bottom = wp.rcNormalPosition.top  + e_sizeMainDialog.cy;
					wp.showCmd                 = (e_bMaximizeMainDialog)
						? SW_MAXIMIZE : SW_RESTORE;
					SetWindowPlacement(hDlg, &wp);
				}
				
				centerParent(hDlg);
				
				SendMessage(hDlg, WM_SIZE, 0, 0);
			}
			return TRUE;
		
		
		// Command
		case WM_COMMAND:
			onMainCommand(LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
			break;
		
		
		case WM_PAINT:
			{
				RECT rc;
				GetClientRect(hDlg, &rc);
				rc.left = rc.right  - GetSystemMetrics(SM_CXHSCROLL);
				rc.top  = rc.bottom - GetSystemMetrics(SM_CYVSCROLL);
				
				RECT rcUpdate;
				if (GetUpdateRect(hDlg, &rcUpdate, TRUE) &&
				    IntersectRect(&rcUpdate, &rc, &rcUpdate)) {
					PAINTSTRUCT ps;
					const HDC hdc = BeginPaint(hDlg, &ps);
					
					DrawFrameControl(hdc, &rc, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
					EndPaint(hDlg, &ps);
				}
			}
			break;
		
		
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
							Shortcut *const psh = (Shortcut*)lvi.lParam;
							if (lvi.mask & LVIF_IMAGE)
								lvi.iImage = psh->getSmallIconIndex();
							if (lvi.mask & LVIF_TEXT) {
								String s;
								psh->getColumnText(lvi.iSubItem, s);
								lstrcpyn(lvi.pszText, s, lvi.cchTextMax);
							}
						}
						break;
					
					case LVN_ITEMCHANGED:
						updateList();
						break;
					
					case LVN_COLUMNCLICK:
						Shortcut::s_iSortColumn = ((const NMLISTVIEW*)lParam)->iSubItem;
						onItemUpdated(~0);
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
					e_acxCol[lvc.iSubItem] = lvc.cx * 100 / rcList.right;
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
						? e_acxCol[lvc.iSubItem] * rcList.right / 100
						: cxRemain;
					cxRemain -= lvc.cx;
					ListView_SetColumn(e_hlst, lvc.iSubItem, &lvc);
				}
				
				RedrawWindow(hDlg, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_NOFRAME);
				
				WINDOWPLACEMENT wp;
				GetWindowPlacement(hDlg, &wp);
				e_sizeMainDialog.cx   = wp.rcNormalPosition.right  - wp.rcNormalPosition.left;
				e_sizeMainDialog.cy   = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
				e_bMaximizeMainDialog = (wp.showCmd == SW_MAXIMIZE);
			}
			break;
		
		
		// Owner draw menu item: populate
		case WM_INITMENUPOPUP:
			if (!HIWORD(lParam)) {
				const HMENU hMenu = (HMENU)wParam;
				MENUITEMINFO mii;
				mii.cbSize = sizeof(mii);
				mii.fMask = MIIM_DATA | MIIM_TYPE;
				mii.cch   = 0;
				if (GetMenuItemCount(hMenu) == 1 &&
				    GetMenuItemInfo(hMenu, 0, TRUE, &mii) &&
				    (mii.fType & MFT_SEPARATOR)) {
					RemoveMenu(hMenu, 0, MF_BYPOSITION);
					((FileMenuItem*)mii.dwItemData)->populate(hMenu);
				}
			}
			break;
		
		// Owner draw menu item: compute size
		case WM_MEASUREITEM:
			{
				MEASUREITEMSTRUCT &mis = *(MEASUREITEMSTRUCT*)lParam;
				if (mis.CtlType == ODT_MENU && mis.itemData) {
					const HDC hdc = GetDC(hDlg);
					((FileMenuItem*)mis.itemData)->measureItem(hdc, mis);
					ReleaseDC(hDlg, hdc);
					return TRUE;
				}
			}
			break;
		
		// Owner draw menu item: draw
		case WM_DRAWITEM:
			{
				DRAWITEMSTRUCT &dis = *(DRAWITEMSTRUCT*)lParam;
				if (dis.CtlType == ODT_MENU && dis.itemData) {
					((FileMenuItem*)dis.itemData)->drawItem(dis);
					return TRUE;
				}
			}
			break;
		
		// End of threadGetFileIcon
		case WM_GETFILEICON:
			if (lParam) {
				GETFILEICON *const pgfi = (GETFILEICON*)lParam;
				
				LVITEM lvi;
				lvi.mask = LVIF_PARAM;
				for (lvi.iItem = ListView_GetItemCount(e_hlst); --lvi.iItem >= 0;) {
					ListView_GetItem(e_hlst, &lvi);
					Shortcut *const psh = (Shortcut*)lvi.lParam;
					if (psh == pgfi->psh)
						psh->onGetFileInfo(*pgfi);
				}
				
				if (pgfi->bAutoDelete)
					delete pgfi;
			}
			break;
	}
	
	return FALSE;
}


LRESULT CALLBACK prcProgramsTarget(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		
		case WM_LBUTTONDOWN:
			if (s_psh) {
				s_bCapturingProgram = true;
				SetCapture(hWnd);
				SetCursor(LoadCursor(e_hInst, MAKEINTRESOURCE(IDC_TARGET)));
				s_sOldPrograms = s_psh->m_sPrograms;
			}
			break;
		
		case WM_MOUSEMOVE:
			if (s_bCapturingProgram && s_psh) {
				POINT pt;
				GetCursorPos(&pt);
				const HWND hWnd = WindowFromPoint(pt);
				
				String rs = s_sOldPrograms;
				DWORD idProcess;
				TCHAR pszPath[MAX_PATH];
				if (GetWindowThreadProcessId(hWnd, &idProcess) &&
				    idProcess != GetCurrentProcessId() &&
				    getWindowExecutable(hWnd, pszPath)) {
					if (rs.isSome())
						rs += _T(';');
					rs += pszPath;
				}
				
				String sOldPrograms;
				getDlgItemText(s_hdlgMain, IDCTXT_PROGRAMS, sOldPrograms);
				if (lstrcmp(sOldPrograms, rs))
					SetDlgItemText(s_hdlgMain, IDCTXT_PROGRAMS, rs);
			}
			break;
		
		case WM_LBUTTONUP:
			if (s_bCapturingProgram) {
				s_bCapturingProgram = false;
				ReleaseCapture();
				if (s_psh) {
					if (lstrcmp(s_psh->m_sPrograms, s_sOldPrograms)) {
						s_psh->cleanPrograms();
						SetDlgItemText(s_hdlgMain, IDCTXT_PROGRAMS, s_psh->m_sPrograms);
					}else
						messageBox(hWnd, ERR_TARGET, MB_ICONEXCLAMATION);
				}
			}
			break;
			
		case WM_CAPTURECHANGED:
			if (s_bCapturingProgram) {
				s_bCapturingProgram = false;
				if (s_psh)
					SetDlgItemText(s_hdlgMain, IDCTXT_PROGRAMS, s_sOldPrograms);
			}
			break;
	}
	
	return CallWindowProc(s_prcProgramsTarget, hWnd, uMsg, wParam, lParam);
}



// Command line shortcut settings
BOOL CALLBACK prcCmdSettings(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM MYUNUSED(lParam))
{
	switch (uMsg) {
		
		case WM_INITDIALOG:
			{
				centerParent(hDlg);
				
				// Fill "command line show" list
				for (int i = 0; i < nbArray(s_aiShow); i++)
					SendDlgItemMessage(hDlg, IDCCBO_SHOW, CB_ADDSTRING, 0, (LPARAM)getToken(tokShowNormal + i));
				
				SetDlgItemText(hDlg, IDCTXT_COMMAND,   s_psh->m_sCommand);
				SetDlgItemText(hDlg, IDCTXT_DIRECTORY, s_psh->m_sDirectory);
				
				int iShow;
				for (iShow = 0; iShow < nbArray(s_aiShow); iShow++)
					if (s_aiShow[iShow] == s_psh->m_nShow)
						break;
				SendDlgItemMessage(hDlg, IDCCBO_SHOW, CB_SETCURSEL, (WPARAM)iShow, 0);
			}
			return TRUE;
		
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				
				// Browse for command line
				case IDCCMD_COMMAND:
					browseForCommandLine(hDlg);
					break;
				
				// Browse for directory
				case IDCCMD_DIRECTORY:
					{
						// Get the command line file (without arguments)
						TCHAR pszDir[MAX_PATH];
						GetDlgItemText(s_hdlgMain, IDCTXT_DIRECTORY, pszDir, nbArray(pszDir));
						
						TCHAR pszTitle[MAX_PATH];
						loadStringAuto(IDS_BROWSEDIR, pszTitle);
						if (!browseForFolder(hDlg, pszTitle, pszDir))
							break;
						
						// Update internal structures
						SetDlgItemText(hDlg, IDCTXT_DIRECTORY, pszDir);
						SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDCTXT_DIRECTORY, EN_KILLFOCUS),
							(LPARAM)GetDlgItem(hDlg, IDCTXT_DIRECTORY));
					}
					break;
				
				case IDOK:
					getDlgItemText(hDlg, IDCTXT_COMMAND,   s_psh->m_sCommand);
					getDlgItemText(hDlg, IDCTXT_DIRECTORY, s_psh->m_sDirectory);
					s_psh->m_nShow = s_aiShow[
						SendDlgItemMessage(hDlg, IDCCBO_SHOW, CB_GETCURSEL, 0,0)];
					s_psh->resetIcons();
					
				case IDCANCEL:
					s_hdlgModal = s_hdlgMain;
					EndDialog(hDlg, LOWORD(wParam));
					break;
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
					ListBox_AddString(hlst, s_asToken[tokLanguageName].get(lang));
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
					setLanguage(ListBox_GetCurSel(hlst));
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
			centerParent(hDlg);
			
			initializeWebLink(hDlg, IDCLBL_WEBSITE, _T("http://utilfr42.free.fr/"));
			initializeWebLink(hDlg, IDCLBL_EMAIL,   _T("mailto:guillaume@ryder.fr"));
			initializeWebLink(hDlg, IDCLBL_DONATE, pszDonateURL);
			SendDlgItemMessage(hDlg, IDCLBL_DONATE, STM_SETIMAGE, IMAGE_BITMAP,
				(LPARAM)loadBitmap(IDB_DONATE));
			return TRUE;
		
		case WM_CTLCOLORSTATIC:
			{
				const int id = GetDlgCtrlID((HWND)lParam);
				if (id == IDCLBL_EMAIL || id == IDCLBL_WEBSITE) {
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
void updateList()
{
	s_bProcessGuiEvents = false;
	
	Shortcut *const psh = getSelShortcut();
	const bool bSel = ToBool(psh);
	
	HICON hIcon = NULL;
	
	if (bSel) {
		// Fill shortcut controls
		
		s_psh = psh;
		CheckRadioButton(s_hdlgMain, IDCOPT_TEXT, IDCOPT_COMMAND,
			(s_psh->m_bCommand) ? IDCOPT_COMMAND : IDCOPT_TEXT);
		
		SetDlgItemText(s_hdlgMain, IDCTXT_TEXT,        s_psh->m_sText);
		SetDlgItemText(s_hdlgMain, IDCTXT_COMMAND,     s_psh->m_sCommand);
		SetDlgItemText(s_hdlgMain, IDCTXT_PROGRAMS,    s_psh->m_sPrograms);
		SetDlgItemText(s_hdlgMain, IDCTXT_DESCRIPTION, s_psh->m_sDescription);
		
		hIcon = s_psh->getIcon();
	}
	
	
	// Enable or disable shortcut controls and Delete button
	
	const bool bCommand = ToBool(IsDlgButtonChecked(s_hdlgMain, IDCOPT_COMMAND));
	static const UINT s_aid[] =
	{
		IDCCMD_DELETE, IDCCMD_EDIT, IDCFRA_TEXT, IDCFRA_COMMAND,
		IDCOPT_TEXT, IDCOPT_COMMAND,
		IDCLBL_DESCRIPTION, IDCTXT_DESCRIPTION,
		IDCLBL_PROGRAMS, IDCTXT_PROGRAMS, IDCIMG_PROGRAMS,
		IDCTXT_TEXT, IDCCMD_TEXT_MENU,
		IDCTXT_COMMAND, IDCCMD_COMMAND, IDCCMD_CMDSETTINGS, IDCCMD_TEST,
	};
	const bool abEnabled[] =
	{
		true, true, true, true,
		true, true,
		true, true,
		true, true, true,
		!bCommand, !bCommand,
		bCommand, bCommand, bCommand, bCommand,
	};
	for (int i = 0; i < nbArray(s_aid); i++)
		EnableWindow(GetDlgItem(s_hdlgMain, s_aid[i]), bSel && abEnabled[i]);
	
	
	// Icon
	SendDlgItemMessage(s_hdlgMain, IDCLBL_ICON, STM_SETICON,
		(WPARAM)((bCommand) ? hIcon : NULL), 0);
	
	s_bProcessGuiEvents = true;
}


// Insert a new item in the shortcut list
// Return its ID
int addItem(Shortcut* psh, bool bSelected)
{
	LVITEM lvi;
	lvi.iItem     = 0;
	lvi.iSubItem  = 0;
	lvi.pszText   = LPSTR_TEXTCALLBACK;
	lvi.mask      = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	lvi.state     = (bSelected) ? LVIS_SELECTED | LVIS_FOCUSED : 0;
	lvi.lParam    = (LPARAM)psh;
	lvi.iImage    = I_IMAGECALLBACK;
	lvi.iItem     = ListView_InsertItem(e_hlst, &lvi);
	lvi.mask      = LVIF_TEXT;
	for (lvi.iSubItem = 1; lvi.iSubItem < nbArray(e_acxCol); lvi.iSubItem++)
		ListView_SetItem(e_hlst, &lvi);
	
	return lvi.iItem;
}


// Called when some columns of an item are updated
void onItemUpdated(int mskColumns)
{
	if (mskColumns & (1 << Shortcut::s_iSortColumn))
		ListView_SortItems(e_hlst, Shortcut::compare, NULL);
	ListView_EnsureVisible(e_hlst, ListView_GetSelectionMark(e_hlst), FALSE);
}



// Handle the tray icon
void trayIconAction(DWORD dwMessage)
{
	VERIFV(e_bIconVisible);
	
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



LPCTSTR getToken(int tok)
{
	return s_asToken[tok].get();
}


// Return a tok* or tokNotFound if not found
int findToken(LPCTSTR pszToken)
{
	for (int tok = 0; tok < nbArray(s_asToken); tok++)
		for (int lang = 0; lang < langCount; lang++)
			if (!lstrcmpi(pszToken, s_asToken[tok].get(lang)))
				return tok;
	
	return tokNotFound;
}


// Find a shortcut in the linked list
// Should not be called while the main dialog box is displayed
Shortcut* findShortcut(BYTE vk, WORD vkFlags, const int aiCondState[], LPCTSTR pszProgram)
{
	for (Shortcut *psh = e_pshFirst; psh; psh = psh->m_pNext)
		if (psh->match(vk, vkFlags, aiCondState, pszProgram))
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

static void skipUntilComma(TCHAR*& rpc);

const LPCTSTR pszSeparator  = _T("-\r\n");


Shortcut::Shortcut(const Keystroke& ks) : Keystroke(ks)
{
	m_pNext      = NULL;
	m_bCommand   = false;
	m_nShow      = SW_NORMAL;
	
	m_iSmallIcon = iconNeeded;
	m_hIcon      = NULL;
}

void Shortcut::save(HANDLE hf)
{
	cleanPrograms();
	
	TCHAR pszHotKey[bufHotKey], pszCode[bufCode];
	getKeyName(pszHotKey);
	wsprintf(pszCode, _T("%lu"), ((DWORD)m_vk) | (((DWORD)m_vkFlags) << 8));
	
	struct LINE
	{
		int     tokKey;
		LPCTSTR pszValue;
	};
	LINE aLine[7 + condTypeCount];
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
	
	String sText;
	
	if (m_bCommand) {
		// Command
		
		aLine[nbLine].tokKey   = tokCommand;
		aLine[nbLine].pszValue = m_sCommand;
		nbLine++;
		
		if (m_sDirectory.isSome()) {
			aLine[nbLine].tokKey   = tokDirectory;
			aLine[nbLine].pszValue = m_sDirectory;
			nbLine++;
		}
		
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
	
	if (m_sPrograms.isSome()) {
		aLine[nbLine].tokKey   = tokPrograms;
		aLine[nbLine].pszValue = m_sPrograms;
		nbLine++;
	}
	
	if (m_sDescription.isSome()) {
		aLine[nbLine].tokKey   = tokDescription;
		aLine[nbLine].pszValue = m_sDescription;
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


void skipUntilComma(TCHAR*& rpc)
{
	TCHAR *pc = rpc;
	while (*pc && *pc != _T(','))
		pc++;
	if (*pc) {
		pc++;
		while (*pc == _T(' '))
			pc++;
	}
	rpc = pc;
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
						if (!lstrcmpi(pcSep, s_asToken[tokLanguageName].get(lang)))
							setLanguage(lang);
				}
				break;
			
			// Main window size
			case tokSize:
				while (*pcSep == _T(' '))
					pcSep++;
				e_sizeMainDialog.cx = StrToInt(pcSep);
				
				skipUntilComma(pcSep);
				e_sizeMainDialog.cy = StrToInt(pcSep);
				
				skipUntilComma(pcSep);
				e_bMaximizeMainDialog = ToBool(StrToInt(pcSep));
				
				skipUntilComma(pcSep);
				e_bIconVisible = !StrToInt(pcSep);
				break;
			
			// Main window columns width
			case tokColumns:
				for (int i = 0; i < colCount - 1; i++) {
					if (!*pcSep)
						break;
					int cx = StrToInt(pcSep);
					if (cx >= 0)
						e_acxCol[i] = cx;
					skipUntilComma(pcSep);
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
			
			// Programs
			case tokPrograms:
				m_sPrograms = pcSep;
				cleanPrograms();
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
	
	// Valid shortcut
	VERIF(m_vk);
	
	// Not twice the same shortcut
	String *const asProgram = getPrograms();
	bool bOK = true;
	for (Shortcut *psh = e_pshFirst; psh; psh = psh->m_pNext)
		if (testConflict(*psh, asProgram)) {
			bOK = false;
			break;
		}
	delete [] asProgram;
	
	return bOK;
}



void Shortcut::findExecutable(LPTSTR pszExecutable)
{
	TCHAR pszFile[MAX_PATH];
	StrCpyN(pszFile, m_sCommand, nbArray(pszFile));
	PathRemoveArgs(pszFile);
	findFullPath(pszFile, pszExecutable);
}


DWORD WINAPI threadGetSmallIcons(void* apgfiParam)
{
	GETFILEICON **const apgfi = (GETFILEICON**)apgfiParam;
	for (int i = 0; apgfi[i]; i++)
		threadGetFileIcon(apgfi[i]);
	delete [] apgfi;
	return 0;
}



DWORD WINAPI threadGetFileIcon(void* pgfi)
{
	GETFILEICON &gfi = *(GETFILEICON*)pgfi;
	
	// Use HTML files icon for URLs
	if (PathIsURL(gfi.pszExecutable)) {
		lstrcpy(gfi.pszExecutable, _T("a.url"));
		gfi.uFlags |= SHGFI_USEFILEATTRIBUTES;
	}
	
	gfi.bOK = getFileInfo(gfi.pszExecutable, 0, gfi.shfi, gfi.uFlags);
	
	PostMessage(s_hdlgMain, WM_GETFILEICON, 0, (LPARAM)pgfi);
	return 0;
}


void Shortcut::onGetFileInfo(GETFILEICON& gfi)
{
	if (gfi.uFlags & SHGFI_SYSICONINDEX) {
		// Small icon index
		
		m_iSmallIcon = (gfi.bOK) ? gfi.shfi.iIcon : iconInvalid;
		
		LVFINDINFO lvfi;
		lvfi.flags  = LVFI_PARAM;
		lvfi.lParam = (LPARAM)gfi.psh;
		const int iItem = ListView_FindItem(e_hlst, -1, &lvfi);
		if (iItem >= 0)
			ListView_RedrawItems(e_hlst, iItem,iItem);
		
	}else{
		// Big icon
		
		m_hIcon = (gfi.bOK) ? gfi.shfi.hIcon : NULL;
		if (s_psh == this)
			SendDlgItemMessage(s_hdlgMain, IDCLBL_ICON, STM_SETICON,
				(WPARAM)((m_bCommand) ? m_hIcon : NULL), 0);
	}
}


void Shortcut::findSmallIconIndex()
{
	VERIFV(m_iSmallIcon != iconThreadRunning);
	m_iSmallIcon = iconThreadRunning;
	
	if (m_bCommand)
		fillGetFileIcon(NULL, true);
}

void Shortcut::fillGetFileIcon(GETFILEICON* pgfi, bool bSmallIcon)
{
	const bool bStart = !pgfi;
	if (bStart)
		pgfi = new GETFILEICON;
	
	if (bSmallIcon)
		m_iSmallIcon = iconThreadRunning;
	
	pgfi->psh         = this;
	pgfi->uFlags      = (bSmallIcon)
		? SHGFI_ICON | SHGFI_SMALLICON | SHGFI_SYSICONINDEX
		: SHGFI_ICON;
	findExecutable(pgfi->pszExecutable);
	pgfi->bAutoDelete = true;
	
	if (bStart) {
		if (*pgfi->pszExecutable)
			startThread(threadGetFileIcon, pgfi);
		else
			threadGetFileIcon(pgfi);
	}
}

void Shortcut::findIcon()
{
	fillGetFileIcon(NULL, false);
}

int Shortcut::getSmallIconIndex()
{
	if (!m_bCommand)
		return -1;
	
	if (m_iSmallIcon == iconNeeded)
		findSmallIconIndex();
	return m_iSmallIcon;
}

void Shortcut::resetIcons()
{
	m_iSmallIcon = iconNeeded;
	if (m_hIcon) {
		DestroyIcon(m_hIcon);
		m_hIcon = NULL;
	}
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


void Shortcut::getColumnText(int iColumn, String& rs) const
{
	switch (iColumn) {
		
		case colContents:
			rs = (m_bCommand) ? m_sCommand : m_sText;
			break;
		
		case colShortcut:
			getKeyName(rs.getBuffer(bufHotKey));
			break;
		
		case colCond:
			{
				static const TCHAR s_acCond[] = { _T('\0'), _T('+'), _T('-') };
				
				for (int cond = 0; cond < 3; cond++)
					if (m_aCond[cond]) {
						rs += s_acCond[m_aCond[cond]];
						rs += s_sCondKeys.get()[cond];
					}
				if (rs.isSome() && m_sPrograms.isSome())
					rs += _T(' ');
				rs += m_sPrograms;
			}
			break;
		
		case 3:
			rs = m_sDescription;
			break;
	}
}



int Shortcut::s_iSortColumn = colContents;

// Compare two shortcuts by shortcut name
int CALLBACK Shortcut::compare(const Shortcut* psh1, const Shortcut* psh2, LPARAM)
{
	switch (s_iSortColumn) {
		case colContents:
			if (psh1->m_bCommand != psh2->m_bCommand)
				return (!psh1->m_bCommand || psh2->m_bCommand) ? +1 : -1;
			break;
		
		case colShortcut:
			{
				const int vkFlags1 = psh1->m_vkFlags;
				const int vkFlags2 = psh2->m_vkFlags;
				if (vkFlags1 != vkFlags2)
					for (int i = 0; i < nbArray(e_aSpecialKey); i++) {
						const int vkFlags = e_aSpecialKey[i].vkFlags;
						const int diff = (int)(vkFlags1 & vkFlags) - (int)(vkFlags2 & vkFlags);
						if (diff)
							return diff;
					}
				
				return (int)psh1->m_vk - (int)psh2->m_vk;
			}
			break;
	}
	
	String s1, s2;
	psh1->getColumnText(s_iSortColumn, s1);
	psh2->getColumnText(s_iSortColumn, s2);
	
	return lstrcmpi(s1, s2);
}



void Shortcut::appendMenuItem(HMENU hMenu, UINT id) const
{
	TCHAR pszKeyName[bufHotKey];
	getKeyName(pszKeyName, false);
	
	TCHAR psz[1024];
	wsprintf(psz, _T("%s\t%s"), pszKeyName, (LPCTSTR)m_sDescription);
	
	AppendMenu(hMenu, (id > idFirstShortcut && ((id - idFirstShortcut) % nbRowMenu) == 0)
		? MF_STRING | MF_MENUBARBREAK : MF_STRING, id, psz);
}



// Return true if we should reset the delay,
// false otherwise.
bool Shortcut::execute() const
{
	if (m_bCommand) {
		// Execute a command line
		
		THREAD_SHELLEXECUTE *const pParams = new THREAD_SHELLEXECUTE(
			m_sCommand, m_sDirectory, m_nShow);
		startThread(threadShellExecute, pParams);
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
				while (*pcEnd && (pcEnd[-1] == _T('\\') || pcEnd[0] != _T(']')))
					pcEnd++;
				String sInside(pcStart, pcEnd - pcStart);
				
				if (sInside.isEmpty()) {
					// Nothing: catch the focus
					
					Keystroke::catchKeyboardFocus(hwndFocus);
					Sleep(100);
					
				}else if (*pcStart == _T('[') && *pcEnd == _T(']')) {
					// Double brackets: [[command line]]
					
					pcEnd++;
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


void fillSpecialCharsMenu(HMENU hMenu, int pos, UINT idFirst)
{
	const HMENU hSubMenu = GetSubMenu(hMenu, pos);
	RemoveMenu(hSubMenu, 0, MF_BYPOSITION);
	for (int i = 128; i < 256; i++) {
		TCHAR pszLabel[30];
		wsprintf(pszLabel, _T("%d - %c"), i, i);
		AppendMenu(hSubMenu,
			((i & 15) == 0) ? MF_STRING | MF_MENUBREAK : MF_STRING,
			idFirst + i, pszLabel);
	}
}


void escapeString(LPCTSTR psz, String& rs)
{
	rs.getBuffer(lstrlen(psz) + 1);
	for (;;)
		switch (*psz) {
			case '\0':
				return;
			case '\\':
			case '[':
			case ']':
				rs += _T('\\');
			default:
				rs += *psz++;
				break;
		}
}


void appendText(LPCTSTR pszText)
{
	VERIFV(s_psh);
	String &rsText = s_psh->m_sText;
	rsText += pszText;
	
	const HWND hctl = GetDlgItem(s_hdlgMain, IDCTXT_TEXT);
	const int len = rsText.getLength();
	Edit_SetText(hctl, rsText);
	Edit_SetSel(hctl, len, len);
	SetFocus(hctl);
}
