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
#include "Shortcut.h"

#ifndef OPENFILENAME_SIZE_VERSION_400
#define OPENFILENAME_SIZE_VERSION_400  sizeof(OPENFILENAME)
#endif

static TranslatedString s_sCondKeys;

namespace shortcut {
static DWORD WINAPI threadGetFilesIcon(GETFILEICON* apgfi[]);
static DWORD WINAPI threadGetFileIcon(GETFILEICON& gfi);
}  // shortcut namespace

struct GETFILEICON {
	Shortcut* psh;
	SHFILEINFO shfi;
	TCHAR pszExecutable[MAX_PATH];
	UINT uFlags;
	bool bOK;
};

namespace dialogs {

static Shortcut *s_psh = NULL;  // Shortcut being edited

static HWND s_hwnd_list = NULL;  // Shortcuts list

// Process GUI events to update shortcuts data from the dialog box controls
static bool s_bProcessGuiEvents;

const LPCTSTR pszValueAutoStart = pszApp;

HWND e_hdlgMain;

const LPCTSTR pszHelpURL = _T("http://utilfr42.free.fr/util/Clavier");
static TCHAR pszDonateURL[256];

static INT_PTR CALLBACK prcMain(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

static INT_PTR CALLBACK prcCmdSettings(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK prcLanguage(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK prcAbout(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK prcProgramsTarget(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static WNDPROC s_prcProgramsTarget;

// ID of the next menu item to add to the "Add" sub-menus (programs and favorites)
static UINT s_idAddMenuId;

static bool browseForCommandLine(HWND hwnd_parent, LPTSTR pszFile, bool bForceExist);
static bool browseForCommandLine(HWND hDlg);
static void onMainCommand(UINT id, WORD wNotify, HWND hWnd);

static Shortcut* getSelectedShortcut();
static void updateItem();
static void updateList();
static int addItem(Shortcut* psh, bool bSelected);
static void onItemUpdated(int mskColumns);
static void onShortcutsCountChanged();

static void fillSpecialCharsMenu(HMENU hMenu, int pos, UINT idFirst);
static void escapeString(LPCTSTR, String& rs);
static void appendText(LPCTSTR pszText);


void initializeCurrentLanguage() {
	s_sCondKeys.load(IDS_CONDITION_KEYS);
}


INT_PTR showMainDialogModal(UINT initial_command) {
	shortcut::GuardList guard;
	return i18n::dialogBox(IDD_MAIN, NULL, prcMain, initial_command);
}


enum {
	sizeposLeft = 1 << 0,
	sizeposLeftHalf = 1 << 1,
	sizeposWidth = 1 << 2,
	sizeposWidthHalf = 1 << 3,
	sizeposTop = 1 << 4,
	sizeposHeight = 1 << 5,
};

struct SIZEPOS {
	int id;
	int flags;
	
	HWND hctl;
	POINT pt;
	SIZE size;
	
	
	void updateGui(int dx, int dy) {
		POINT ptNew = pt;
		SIZE sizeNew = size;
		
		if (flags & sizeposLeft) {
			ptNew.x += dx;
		} else if (flags & sizeposLeftHalf) {
			ptNew.x += dx / 2;
		}
		
		if (flags & sizeposWidth) {
			sizeNew.cx += dx;
		} else if (flags & sizeposWidthHalf) {
			sizeNew.cx += dx / 2;
		}
		
		if (flags & sizeposTop) {
			ptNew.y += dy;
		}
		
		if (flags & sizeposHeight) {
			sizeNew.cy += dy;
		}
		
		MoveWindow(hctl, ptNew.x, ptNew.y, sizeNew.cx, sizeNew.cy, FALSE);
	}
};

static SIZEPOS aSizePos[] = {
	{ IDCLST, sizeposWidth | sizeposHeight },
	
	{ IDCCMD_ADD, sizeposTop },
	{ IDCCMD_DELETE, sizeposTop },
	{ IDCCMD_EDIT, sizeposTop },
	{ IDCLBL_HELP, sizeposTop },
	{ IDCLBL_SHORTCUTSCOUNT, sizeposTop },
	{ IDCTXT_SHORTCUTSCOUNT, sizeposTop },
	{ IDCCHK_AUTOSTART, sizeposTop },
	
	{ IDCOPT_TEXT, sizeposTop },
	{ IDCFRA_TEXT, sizeposTop | sizeposWidthHalf },
	{ IDCTXT_TEXT, sizeposTop | sizeposWidthHalf },
	{ IDCCMD_TEXT_MENU, sizeposTop | sizeposLeftHalf },
	
	{ IDCOPT_COMMAND, sizeposTop | sizeposLeftHalf },
	{ IDCFRA_COMMAND, sizeposTop | sizeposLeftHalf | sizeposWidthHalf },
	{ IDCTXT_COMMAND, sizeposTop | sizeposLeftHalf | sizeposWidthHalf },
	{ IDCCMD_COMMAND, sizeposTop | sizeposLeft },
	{ IDCLBL_ICON, sizeposTop | sizeposLeftHalf },
	{ IDCCMD_CMDSETTINGS, sizeposTop | sizeposLeftHalf },
	{ IDCCMD_TEST, sizeposTop | sizeposLeftHalf },
	
	{ IDCLBL_PROGRAMS, sizeposTop },
	{ IDCCBO_PROGRAMS, sizeposTop },
	{ IDCTXT_PROGRAMS, sizeposTop | sizeposWidth },
	{ IDCIMG_PROGRAMS, sizeposTop | sizeposLeft },
	
	{ IDCLBL_DESCRIPTION, sizeposTop },
	{ IDCTXT_DESCRIPTION, sizeposTop | sizeposWidth },
	
	{ IDCLBL_DONATE, sizeposTop },
	
	{ IDCCMD_HELP, sizeposTop | sizeposLeft },
	{ IDCCMD_COPYLIST, sizeposTop | sizeposLeft },
	{ IDCCMD_LANGUAGE, sizeposTop | sizeposLeft },
	{ IDCCMD_ABOUT, sizeposTop | sizeposLeft },
	{ IDCCMD_QUIT, sizeposTop | sizeposLeft },
	{ IDCANCEL, sizeposTop | sizeposLeft },
	{ IDOK, sizeposTop | sizeposLeft },
};


static void createAndAddShortcut(LPCTSTR pszCommand = NULL, bool bSupportFileOpen = false);
static Shortcut* createShortcut();
static void addCreatedShortcut(Shortcut* psh);


void createAndAddShortcut(LPCTSTR pszCommand, bool bSupportFileOpen) {
	Shortcut *const psh = createShortcut();
	if (psh) {
		if (pszCommand) {
			psh->m_bCommand = true;
			psh->m_sCommand = pszCommand;
			PathQuoteSpaces(psh->m_sCommand.getBuffer(MAX_PATH));
		}
		psh->m_bSupportFileOpen = bSupportFileOpen;
		addCreatedShortcut(psh);
	}
}

Shortcut* createShortcut() {
	SetFocus(s_hwnd_list);
	Keystroke ks;
	return (askKeystroke(dialogs::e_hdlgMain, NULL, ks))
		? new Shortcut(ks) : NULL;
}

void addCreatedShortcut(Shortcut* psh) {
	addItem(psh, true);
	onItemUpdated(~0);
	updateList();
	onShortcutsCountChanged();
	
	const HWND hWnd = GetDlgItem(e_hdlgMain, (psh->m_bCommand) ? IDCTXT_COMMAND : IDCTXT_TEXT);
	if (psh->m_bCommand) {
		const int len = psh->m_sCommand.getLength();
		Edit_SetSel(hWnd, len, len);
	}
	SetFocus(hWnd);
}


class FileMenuItem {
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
	
	bool m_bStartMenu;
	bool m_bIsDir;
	String m_sLabel;
	String m_sPath;
	int m_iconIndex;
	
	enum{ cxLeft = 3, cxBetween = 5, cxRight = 16 };
};


static FileMenuItem* findItemAndCleanMenu(HMENU hMenu, UINT id);

static HIMAGELIST s_hSysImageList = NULL;


FileMenuItem* findItemAndCleanMenu(HMENU hMenu, UINT id) {
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_ID | MIIM_SUBMENU;
	
	FileMenuItem *pItemToExecute = NULL;
	for (UINT pos = 0; GetMenuItemInfo(hMenu, pos, TRUE, &mii); pos++) {
		FileMenuItem *pItem = (FileMenuItem*)mii.dwItemData;
		if (pItem) {
			if (mii.wID == id && id) {
				pItemToExecute = pItem;
			} else if (!(mii.fType & MFT_SEPARATOR)) {
				delete pItem;
			}
		}
		if (mii.hSubMenu) {
			pItem = findItemAndCleanMenu(mii.hSubMenu, id);
			if (pItem && !pItemToExecute) {
				pItemToExecute = pItem;
			}
		}
	}
	
	return pItemToExecute;
}



#pragma warning(disable: 4701)
void FileMenuItem::populate(HMENU hMenu) {
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fType = MFT_OWNERDRAW;
	
	MENUITEMINFO miiSub;
	miiSub.cbSize = sizeof(miiSub);
	miiSub.fMask = MIIM_TYPE | MIIM_DATA;
	miiSub.fType = MFT_SEPARATOR;
	miiSub.dwTypeData = _T("");
	
	MENUITEMINFO miiParam;
	miiParam.cbSize = sizeof(mii);
	miiParam.fMask = MIIM_DATA | MIIM_TYPE;
	miiParam.cch = 0;
	
	UINT posFolder = 0;
	
	for (int iSource = 0; iSource < 2; iSource++) {
		TCHAR pszFolder[MAX_PATH];
		
		if (m_bStartMenu) {
			if (!getSpecialFolderPath((iSource) ? CSIDL_COMMON_STARTMENU : CSIDL_STARTMENU, pszFolder)) {
				continue;
			}
			if (m_sPath.isSome()) {
				PathAppend(pszFolder, m_sPath);
			}
		} else if (iSource > 0) {
			break;
		} else {
			lstrcpyn(pszFolder, m_sPath, arrayLength(pszFolder));
		}
		
		TCHAR pszPath[MAX_PATH];
		PathCombine(pszPath, pszFolder, _T("*"));
		WIN32_FIND_DATA wfd;
		const HANDLE hff = FindFirstFile(pszPath, &wfd);
		if (hff == INVALID_HANDLE_VALUE) {
			continue;
		}
		do {
			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) {
				continue;
			}
			PathCombine(pszPath, pszFolder, wfd.cFileName);
			
			TCHAR pszRelativeFolder[MAX_PATH];
			LPTSTR pszItemPath;
			
			const bool bIsDir = toBool(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
			
			// Directories: submenus
			// Files: only *.lnk files
			UINT pos;
			if (bIsDir) {
				if (wfd.cFileName[0] == _T('.') &&
						(wfd.cFileName[1] == _T('.') || !wfd.cFileName[1])) {
					continue;
				}
				
				lstrcpyn(pszRelativeFolder, m_sPath, arrayLength(pszRelativeFolder));
				PathAppend(pszRelativeFolder, wfd.cFileName);
				pszItemPath = pszRelativeFolder;
				
				if (iSource) {
					const int nbItem = GetMenuItemCount(hMenu);
					for (int i = 0; i < nbItem; i++) {
						if (!GetMenuItemInfo(hMenu, i, TRUE, &mii)) {
							continue;
						}
						const FileMenuItem &fmiOther = *(const FileMenuItem*)mii.dwItemData;
						if (fmiOther.isDir() && !lstrcmpi(fmiOther.m_sPath, pszRelativeFolder)) {
							goto Next;
						}
					}
				}
				
				mii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_SUBMENU;
				mii.hSubMenu = CreatePopupMenu();
				pos = posFolder++;
				
			} else {
				mii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_ID;
				mii.wID = s_idAddMenuId++;
				pos = (UINT)-1;
				PathRemoveExtension(wfd.cFileName);
				pszItemPath = pszPath;
			}
			
			// Get the small icon
			SHFILEINFO shfi;
			if (!getFileInfo(pszPath, wfd.dwFileAttributes, shfi,
					SHGFI_ICON | SHGFI_SMALLICON | SHGFI_SYSICONINDEX)) {
				shfi.iIcon = -1;
			}
			
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
		} while (FindNextFile(hff, &wfd));
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


void FileMenuItem::measureItem(HDC hdc, MEASUREITEMSTRUCT& mis) {
	SelectFont(hdc, GetStockFont(DEFAULT_GUI_FONT));
	
	RECT rc;
	DrawText(hdc, m_sLabel, -1, &rc,
		DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_CALCRECT);
	
	mis.itemWidth = cxLeft + GetSystemMetrics(SM_CXSMICON) + cxBetween + rc.right - rc.left + cxRight;
	mis.itemHeight = rc.bottom - rc.top;
	int cyMenu = GetSystemMetrics(SM_CYMENU);
	int cyIcon = GetSystemMetrics(SM_CXSMICON);
	if (cyMenu < cyIcon) {
		cyMenu = cyIcon;
	}
	if ((int)mis.itemHeight < cyMenu) {
		mis.itemHeight = cyMenu;
	}
}


void FileMenuItem::drawItem(DRAWITEMSTRUCT& dis) {
	int sysColorText, sysColorBack;
	if (dis.itemState & ODS_SELECTED) {
		sysColorText = COLOR_HIGHLIGHTTEXT;
		sysColorBack = COLOR_HIGHLIGHT;
	} else {
		sysColorText = COLOR_MENUTEXT;
		sysColorBack = COLOR_MENU;
	}
	
	COLORREF clTextOld = SetTextColor(dis.hDC, GetSysColor(sysColorText));
	COLORREF clBackOld = SetBkColor(dis.hDC, GetSysColor(sysColorBack));
	ExtTextOut(dis.hDC, dis.rcItem.left, dis.rcItem.top,
		ETO_CLIPPED | ETO_OPAQUE, &dis.rcItem, _T(""), 0, NULL);
	
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
	SetBkColor(dis.hDC, clBackOld);
}


void FileMenuItem::execute() {
	TCHAR pszPath[MAX_PATH];
	if (!getShellLinkTarget(m_sPath, pszPath)) {
		lstrcpy(pszPath, m_sPath);
	}
	createAndAddShortcut(pszPath);
}


bool browseForCommandLine(HWND hwnd_parent, LPTSTR pszFile, bool bForceExist) {
	PathUnquoteSpaces(pszFile);
	
	// Ask for the file
	OPENFILENAME ofn;
	ZeroMemory(&ofn, OPENFILENAME_SIZE_VERSION_400);
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner = hwnd_parent;
	ofn.lpstrFile = pszFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = (bForceExist) ? OFN_FILEMUSTEXIST | OFN_HIDEREADONLY : OFN_HIDEREADONLY;
	VERIF(GetOpenFileName(&ofn));
	
	PathQuoteSpaces(pszFile);
	return true;
}


bool browseForCommandLine(HWND hDlg) {
	TCHAR pszFile[MAX_PATH];
	GetDlgItemText(hDlg, IDCTXT_COMMAND, pszFile, arrayLength(pszFile));
	PathRemoveArgs(pszFile);
	
	VERIF(browseForCommandLine(hDlg, pszFile, true));
	
	SetDlgItemText(hDlg, IDCTXT_COMMAND, pszFile);
	return true;
}


static bool s_bCapturingProgram = false;
static String s_sOldPrograms;

static const LPCTSTR apszWrite[] = {
	_T("[|...|]"),
	_T("[]"),
	_T("\\\\"),
	_T("\\["),
	_T("\\]"),
	_T("\\{"),
	_T("\\}"),
	_T("\\|"),
	_T("[{Wait,<duration>}]"),
	_T("[{Focus,<delay>,<window name>}]"),
	_T("[{Copy,<text>}]"),
	_T("[{MouseButton,L/M/R(U/D)}]"),
	_T("[{MouseMoveTo,x,y}]"),
	_T("[{MouseMoveToFocus,x,y}]"),
	_T("[{MouseMoveBy,dx,dy}]"),
	_T("[{MouseWheel,<+/- ticks>}]"),
};


void onMainCommand(UINT id, WORD wNotify, HWND hWnd) {
	switch (id) {
		
		case IDCANCEL:
			{
				// Get delete new shortcuts
				LVITEM lvi;
				lvi.mask = LVIF_PARAM;
				for (lvi.iItem = 0; ListView_GetItem(s_hwnd_list, &lvi); lvi.iItem++) {
					delete (Shortcut*)lvi.lParam;
				}
				ListView_DeleteAllItems(s_hwnd_list);
				EndDialog(e_hdlgMain, IDCANCEL);
			}
			break;
		
		case IDCCMD_QUIT:
		case IDOK:
		case IDCCMD_LANGUAGE:
			{
				// Test for cancellation before doing anything else.
				if (id == IDCCMD_LANGUAGE) {
					if (i18n::dialogBox(IDD_LANGUAGE, e_hdlgModal, prcLanguage) == IDCANCEL) {
						break;
					}
				} else if (id == IDCCMD_QUIT) {
					if (!isKeyDown(VK_SHIFT) && IDNO ==
							messageBox(e_hdlgMain, ASK_QUIT, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION)) {
						break;
					}
				}
				
				// Get all shortcuts from the list.
				const int nbItem = ListView_GetItemCount(s_hwnd_list);
				Shortcut **const ash = new Shortcut*[nbItem];
				LVITEM lvi;
				lvi.mask = LVIF_PARAM;
				for (lvi.iItem = 0; lvi.iItem < nbItem; lvi.iItem++) {
					ListView_GetItem(s_hwnd_list, &lvi);
					ash[lvi.iItem] = reinterpret_cast<Shortcut*>(lvi.lParam);
				}
				
				// Create a valid linked list from the list box
				// and register the hot keys
				shortcut::clearShortcuts();
				for (int i = nbItem; --i >= 0;) {
					Shortcut *const psh = ash[i];
					psh->resetIcons();
					psh->addToList();
				}
				delete [] ash;
				
				if (id != IDCCMD_QUIT) {
					shortcut::saveShortcuts();
				}
				
				// Create/delete the auto start key
				TCHAR pszAutoStartPath[MAX_PATH];
				const HKEY hKeyAutoStart = openAutoStartKey(pszAutoStartPath);
				if (hKeyAutoStart) {
					if (IsDlgButtonChecked(e_hdlgMain, IDCCHK_AUTOSTART)) {
						RegSetValueEx(hKeyAutoStart, pszValueAutoStart, NULL, REG_SZ,
							(const BYTE*)pszAutoStartPath, lstrlen(pszAutoStartPath) + 1);
					} else {
						RegDeleteValue(hKeyAutoStart, pszValueAutoStart);
					}
					RegCloseKey(hKeyAutoStart);
				}
			}
			
			EndDialog(e_hdlgMain, id);
			break;
		
		// Display "Add" menu
		case IDCCMD_ADD:
			{
				const HMENU hAllMenus = i18n::loadMenu(IDM_CONTEXT);
				
				HMENU hMenu = GetSubMenu(hAllMenus, 0);
				TCHAR pszFolder[MAX_PATH];
				
				s_idAddMenuId = ID_ADD_PROGRAM_FIRST;
				
				MENUITEMINFO mii;
				mii.cbSize = sizeof(mii);
				mii.fMask = MIIM_DATA;
				
				MENUITEMINFO miiSub;
				miiSub.cbSize = sizeof(miiSub);
				miiSub.fMask = MIIM_TYPE | MIIM_DATA;
				miiSub.fType = MFT_SEPARATOR;
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
					rc.left, rc.bottom, 0, e_hdlgMain, NULL);
				FileMenuItem *const pItem = findItemAndCleanMenu(hMenu, id);
				DestroyMenu(hAllMenus);
				
				if (pItem) {
					pItem->execute();
					delete pItem;
				} else if (id) {
					SendMessage(e_hdlgMain, WM_COMMAND, id, 0);
				}
			}
			break;
		
		case ID_ADD_FOLDER:
			{
				TCHAR pszFolder[MAX_PATH] = _T("");
				if (browseForFolder(e_hdlgMain, NULL, pszFolder)) {
					createAndAddShortcut(pszFolder, true);
				}
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
				if (!isKeyDown(VK_SHIFT) && IDNO == messageBox(
						e_hdlgMain, ASK_DELETE, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION)) {
					break;
				}
				
				SetFocus(GetDlgItem(e_hdlgMain, IDCCMD_ADD));
				SendMessage(e_hdlgMain, WM_NEXTDLGCTL, TRUE, FALSE);
				
				LVITEM lvi;
				lvi.iItem = ListView_GetNextItem(s_hwnd_list, -1, LVNI_SELECTED);
				if (lvi.iItem >= 0) {
					lvi.mask = LVIF_PARAM;
					ListView_GetItem(s_hwnd_list, &lvi);
					
					// Delete item, hotkey, and shortcut object
					ListView_DeleteItem(s_hwnd_list, lvi.iItem);
					delete (Shortcut*)lvi.lParam;
				}
				
				// Select an other item and update
				if (lvi.iItem >= ListView_GetItemCount(s_hwnd_list)) {
					lvi.iItem--;
				}
				ListView_SetItemState(s_hwnd_list, lvi.iItem,
						LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
				updateList();
				onShortcutsCountChanged();
				SetFocus(s_hwnd_list);
			}
			break;
		
		case IDCCMD_EDIT:
			SetFocus(s_hwnd_list);
			{
				Keystroke ks = *s_psh;
				if (askKeystroke(e_hdlgMain, s_psh, ks)) {
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
				if (Keystroke::askSendKeys(e_hdlgMain, ks)) {
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
				GetModuleFileName(e_hInst, pszPath, arrayLength(pszPath));
				PathRemoveFileSpec(pszPath);
				
				TCHAR pszFile[MAX_PATH];
				i18n::loadStringAuto(IDS_HELP, pszFile);
				PathAppend(pszPath, pszFile);
				
				if (32 >= reinterpret_cast<UINT_PTR>(
						ShellExecute(NULL, NULL, pszPath, NULL, NULL, SW_SHOWDEFAULT))) {
					lstrcpy(pszPath, pszHelpURL);
					lstrcat(pszPath, pszFile);
					ShellExecute(NULL, NULL, pszPath, NULL, NULL, SW_SHOWDEFAULT);
				}
			}
			break;
		
		
		case IDCCMD_ABOUT:
			i18n::dialogBox(IDD_ABOUT, e_hdlgMain, prcAbout);
			break;
		
		
		case IDCCMD_COPYLIST:
			{
				String s;
				LVITEM lvi;
				lvi.mask = LVIF_PARAM;
				for (lvi.iItem = 0;; lvi.iItem++) {
					if (!ListView_GetItem(s_hwnd_list, &lvi)) {
						break;
					}
					reinterpret_cast<const Shortcut*>(lvi.lParam)->appendCsvLineToString(s);
				}
				shortcut::copyShortcutsToClipboard(s);
				
				messageBox(e_hdlgMain, MSG_COPYLIST, MB_ICONINFORMATION);
			}
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
				SetFocus(GetDlgItem(e_hdlgMain, (bCommand) ? IDCTXT_COMMAND : IDCTXT_TEXT));
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
				String* ps;
				switch (id) {
					case IDCTXT_TEXT:
						ps = &s_psh->m_sText;
						break;
					case IDCTXT_COMMAND:
						ps = &s_psh->m_sCommand;
						iColumn = colContents;
						break;
					case IDCTXT_PROGRAMS:
						ps = &s_psh->m_sPrograms;
						iColumn = colCond;
						break;
					default:  // case IDCTXT_DESCRIPTION
						ps = &s_psh->m_sDescription;
						iColumn = colDescription;
						break;
				}
				
				getDlgItemText(e_hdlgMain, id, *ps);
				
				if (id == IDCTXT_COMMAND) {
					s_psh->resetIcons();
					s_psh->getIcon();
				}
				
				updateItem();
				
				if (iColumn >= 0) {
					onItemUpdated(1 << iColumn);
				}
			}
			break;
		
		case IDCCBO_PROGRAMS:
			if (wNotify == CBN_SELCHANGE) {
				s_psh->m_bProgramsOnly = toBool(SendDlgItemMessage(e_hdlgMain,
					IDCCBO_PROGRAMS, CB_GETCURSEL, 0, 0));
				updateItem();
			}
			break;
		
		// Browse for command line
		case IDCCMD_COMMAND:
			if (browseForCommandLine(e_hdlgMain)) {
				SendMessage(e_hdlgMain, WM_COMMAND, MAKEWPARAM(IDCTXT_COMMAND, EN_KILLFOCUS),
					(LPARAM)GetDlgItem(e_hdlgMain, IDCTXT_COMMAND));
				updateItem();
			}
			break;
		
		case IDCCMD_CMDSETTINGS:
			if (IDOK == i18n::dialogBox(IDD_CMDSETTINGS, e_hdlgMain, prcCmdSettings)) {
				updateList();
				updateItem();
				onItemUpdated(1 << colContents);
			}
			break;
		
		case IDCCMD_TEST:
			s_psh->execute(false);
			break;
		
		case IDCCMD_TEXT_MENU:
			{
				const HMENU hAllMenus = i18n::loadMenu(IDM_CONTEXT);
				const HMENU hMenu = GetSubMenu(hAllMenus, 1);
				
				fillSpecialCharsMenu(hMenu, 5, ID_TEXT_SPECIALCHAR_FIRST);
				
				RECT rc;
				GetWindowRect(hWnd, &rc);
				SetFocus(GetDlgItem(e_hdlgMain, IDCTXT_TEXT));
				TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON,
					rc.left, rc.bottom, 0, e_hdlgMain, NULL);
				
				DestroyMenu(hAllMenus);
			}
			break;
		
		case ID_TEXT_COMMAND:
			{
				TCHAR pszFile[MAX_PATH] = _T("");
				if (browseForCommandLine(e_hdlgMain, pszFile, false)) {
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
					psh->m_sText = pszText;
					addCreatedShortcut(psh);
				}
			} else if (ID_TEXT_SPECIALCHAR_FIRST <= id && id < ID_TEXT_SPECIALCHAR_FIRST + 256) {
				const TCHAR pszText[] = { (TCHAR)(id - ID_TEXT_SPECIALCHAR_FIRST), _T('\0') };
				String s;
				escapeString(pszText, s);
				appendText(s);
			} else if (ID_TEXT_LOWLEVEL <= id && id < ID_TEXT_LOWLEVEL + arrayLength(apszWrite)) {
				appendText(apszWrite[id - ID_TEXT_LOWLEVEL]);
			}
			break;
	}
}


// Main dialog box
INT_PTR CALLBACK prcMain(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static RECT rcClientOrig;
	static SIZE sizeMinimum;
	
	switch (uMsg) {
		
		// Initialization
		case WM_INITDIALOG:
			{
				e_hdlgMain = e_hdlgModal = hDlg;
				
				s_bProcessGuiEvents = false;
				
				s_prcProgramsTarget = subclassWindow(GetDlgItem(hDlg, IDCIMG_PROGRAMS), prcProgramsTarget);
				
				s_hwnd_list = GetDlgItem(hDlg, IDCLST);
				ListView_SetExtendedListViewStyle(s_hwnd_list, LVS_EX_FULLROWSELECT);
				
				// Get the initial size
				GetWindowRect(hDlg, &rcClientOrig);
				sizeMinimum.cx = rcClientOrig.right - rcClientOrig.left;
				sizeMinimum.cy = rcClientOrig.bottom - rcClientOrig.top;
				GetClientRect(hDlg, &rcClientOrig);
				
				// Get the initial position of controls
				for (int i = 0; i < arrayLength(aSizePos); i++) {
					SIZEPOS &sp = aSizePos[i];
					sp.hctl = GetDlgItem(hDlg, sp.id);
					RECT rc;
					GetWindowRect(sp.hctl, &rc);
					sp.size.cx = rc.right - rc.left;
					sp.size.cy = rc.bottom - rc.top;
					ScreenToClient(hDlg, (POINT*)&rc);
					sp.pt = (POINT&)rc;
				}
				
				i18n::loadStringAuto(IDS_DONATEURL, pszDonateURL);
				initializeWebLink(hDlg, IDCLBL_DONATE, pszDonateURL);
				SendDlgItemMessage(hDlg, IDCLBL_DONATE, STM_SETIMAGE, IMAGE_BITMAP,
					(LPARAM)i18n::loadBitmap(IDB_DONATE));
				
				SendDlgItemMessage(hDlg, IDCCMD_TEXT_MENU, BM_SETIMAGE, IMAGE_BITMAP,
					(LPARAM)LoadBitmap(NULL, MAKEINTRESOURCE(OBM_COMBO)));
				
				// Get the handle of the system image list for small icons
				SHFILEINFO shfi;
				s_hSysImageList = (HIMAGELIST)SHGetFileInfo(
					_T("a"), FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(shfi),
					SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_SMALLICON | SHGFI_SYSICONINDEX);
				ListView_SetImageList(s_hwnd_list, s_hSysImageList, LVSIL_SMALL);
				
				TCHAR pszText[512];
				
				// Add columns to the list
				i18n::loadStringAuto(IDS_COLUMNS, pszText);
				TCHAR *pcText = pszText;
				LVCOLUMN lvc;
				lvc.mask = LVCF_TEXT | LVCF_SUBITEM;
				for (lvc.iSubItem = 0; lvc.iSubItem < colCount; lvc.iSubItem++) {
					lvc.pszText = pcText;
					while (*pcText != _T(';')) {
						pcText++;
					}
					*pcText++ = _T('\0');
					ListView_InsertColumn(s_hwnd_list, lvc.iSubItem, &lvc);
				}
				
				// Fill shortcuts list and unregister hot keys
				int nbShortcut = 0, nbShortcutIcon = 0;
				for (Shortcut *psh = shortcut::getFirst(); psh; psh = psh->getNext()) {
					Shortcut *const pshCopy = new Shortcut(*psh);
					addItem(pshCopy, (s_psh == psh));
					nbShortcut++;
					if (pshCopy->m_bCommand) {
						nbShortcutIcon++;
					}
				}
				onItemUpdated(~0);
				onShortcutsCountChanged();
				
				// Load small icons
				GETFILEICON **const apgfi = new GETFILEICON*[nbShortcutIcon + 1];
				apgfi[nbShortcutIcon] = NULL;
				LVITEM lvi;
				lvi.mask = LVIF_PARAM;
				for (lvi.iItem = 0; lvi.iItem < nbShortcut ; lvi.iItem++) {
					ListView_GetItem(s_hwnd_list, &lvi);
					Shortcut *const psh = (Shortcut*)lvi.lParam;
					if (psh->m_bCommand) {
						psh->fillGetFileIcon(apgfi[--nbShortcutIcon] = new GETFILEICON, true);
					}
				}
				startThread((LPTHREAD_START_ROUTINE)shortcut::threadGetFilesIcon, apgfi);
				
				// Programs combo box
				const HWND hcboPrograms = GetDlgItem(hDlg, IDCCBO_PROGRAMS);
				TCHAR pszPrograms[bufString];
				i18n::loadStringAuto(IDS_PROGRAMS, pszPrograms);
				TCHAR *pcStart = pszPrograms;
				for (int i = 0; i < 2; i++) {
					TCHAR *pc = pcStart;
					while (*pc != _T(';')) {
						pc++;
					}
					*pc = _T('\0');
					SendMessage(hcboPrograms, CB_ADDSTRING, 0, (LPARAM)pcStart);
					pcStart = pc + 1;
				}
				
				updateList();
				
				// Button icons and tooltips
				const HWND hwndTT = CreateWindowEx(WS_EX_TOPMOST,
					TOOLTIPS_CLASS, NULL,
					WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
					CW_USEDEFAULT, CW_USEDEFAULT,
					CW_USEDEFAULT, CW_USEDEFAULT,
					hDlg, NULL, e_hInst, NULL);
				SetWindowPos(hwndTT,
					HWND_TOPMOST, 0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				
				i18n::loadStringAuto(IDS_TOOLTIPS, pszText);
				pcText = pszText;
				TOOLINFO ti;
				ti.cbSize = sizeof(ti);
				ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
				ti.hinst = e_hInst;
				for (int i = 0; i < 4; i++) {
					const UINT id = (i < 3) ? IDCCMD_ADD + i : IDCIMG_PROGRAMS;
					
					const HWND hctl = GetDlgItem(hDlg, id);
					if (i < 3) {
						SendMessage(hctl, BM_SETIMAGE, IMAGE_ICON,
								(LPARAM)(HICON)LoadImage(e_hInst,
										MAKEINTRESOURCE(IDI_ADD + i), IMAGE_ICON, 15,15, 0));
					}
					
					ti.hwnd = hctl;
					ti.uId = reinterpret_cast<UINT_PTR>(hctl);
					ti.lpszText = pcText;
					while (*pcText != _T(';')) {
						pcText++;
					}
					*pcText++ = _T('\0');
					SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)&ti);
				}
				
				// Auto start
				TCHAR pszAutoStartPath[MAX_PATH];
				const HKEY hKeyAutoStart = openAutoStartKey(pszAutoStartPath);
				if (hKeyAutoStart) {
					DWORD dwType, dwLen;
					TCHAR pszPath[MAX_PATH];
					
					if (ERROR_SUCCESS == RegQueryValueEx(hKeyAutoStart, pszValueAutoStart, NULL,
							&dwType, NULL, &dwLen) && dwType == REG_SZ && dwLen < arrayLength(pszPath)) {
						if (ERROR_SUCCESS == RegQueryValueEx(hKeyAutoStart, pszValueAutoStart, NULL,
								NULL, (BYTE*)pszPath, &dwLen) && !lstrcmpi(pszPath, pszAutoStartPath)) {
							CheckDlgButton(hDlg, IDCCHK_AUTOSTART, TRUE);
						}
					}
					
					RegCloseKey(hKeyAutoStart);
				}
				
				s_bProcessGuiEvents = true;
				
				SendMessage(hDlg, WM_SETICON, FALSE,
						(LPARAM)(HICON)LoadImage(e_hInst, MAKEINTRESOURCE(IDI_APP), IMAGE_ICON, 16,16, 0));
				SendMessage(hDlg, WM_SETICON, TRUE,
						(LPARAM)LoadIcon(e_hInst, MAKEINTRESOURCE(IDI_APP)));
				
				// Restore window size from settings
				if (e_sizeMainDialog.cx < sizeMinimum.cx || e_sizeMainDialog.cy < sizeMinimum.cy) {
					e_sizeMainDialog = sizeMinimum;
				} else{
					// Default settings: center window in the default monitor.
					
					const HMONITOR hMonitor = MonitorFromWindow(hDlg, MONITOR_DEFAULTTOPRIMARY);
					MONITORINFO mi;
					mi.cbSize = sizeof(mi);
					GetMonitorInfo(hMonitor, &mi);
					
					WINDOWPLACEMENT wp;
					GetWindowPlacement(hDlg, &wp);
					wp.rcNormalPosition.left = (mi.rcWork.left + mi.rcWork.right - e_sizeMainDialog.cx) / 2;
					wp.rcNormalPosition.top = (mi.rcWork.top + mi.rcWork.bottom - e_sizeMainDialog.cy) / 2;
					wp.rcNormalPosition.right = wp.rcNormalPosition.left + e_sizeMainDialog.cx;
					wp.rcNormalPosition.bottom = wp.rcNormalPosition.top + e_sizeMainDialog.cy;
					wp.showCmd = (e_bMaximizeMainDialog) ? SW_MAXIMIZE : SW_RESTORE;
					SetWindowPlacement(hDlg, &wp);
				}
				
				centerParent(hDlg);
				
				SendMessage(hDlg, WM_SIZE, 0, 0);
				
				if (lParam) {
					PostMessage(hDlg, WM_COMMAND, (WPARAM)lParam, 0);
				}
			}
			return TRUE;
		
		
		// Command
		case WM_COMMAND:
			onMainCommand(LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
			break;
		
		
		// Draw the bottom-right gripper
		case WM_PAINT:
			{
				RECT rc;
				GetClientRect(hDlg, &rc);
				rc.left = rc.right - GetSystemMetrics(SM_CXHSCROLL);
				rc.top = rc.bottom - GetSystemMetrics(SM_CYVSCROLL);
				
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
		
		// Behavior of the bottom-right gripper:
		// same as the bottom-right corner of the dialog box
		case WM_NCHITTEST:
			{
				RECT rc;
				GetClientRect(hDlg, &rc);
				ClientToScreen(hDlg, (POINT*)&rc + 1);
				rc.left = rc.right - GetSystemMetrics(SM_CXHSCROLL);
				rc.top = rc.bottom - GetSystemMetrics(SM_CYVSCROLL);
				
				const POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				if (PtInRect(&rc, pt)) {
					setWindowLongPtr(hDlg, DWL_MSGRESULT, HTBOTTOMRIGHT);
					return TRUE;
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
							if (lvi.mask & LVIF_IMAGE) {
								lvi.iImage = psh->getSmallIconIndex();
							}
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
				
			} else if (wParam == 0 && ((NMHDR*)lParam)->code == HDN_ENDTRACK) {
				// Get columns size
				LVCOLUMN lvc;
				lvc.mask = LVCF_WIDTH;
				RECT rcList;
				GetClientRect(s_hwnd_list, &rcList);
				for (lvc.iSubItem = 0; lvc.iSubItem < nbColSize; lvc.iSubItem++) {
					ListView_GetColumn(s_hwnd_list, lvc.iSubItem, &lvc);
					e_acxCol[lvc.iSubItem] = MulDiv(lvc.cx, 100, rcList.right);
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
				int dx = rcClientNew.right - rcClientOrig.right;
				int dy = rcClientNew.bottom - rcClientOrig.bottom;
				
				for (int i = 0; i < arrayLength(aSizePos); i++) {
					aSizePos[i].updateGui(dx, dy);
				}
				
				// Resize columns
				LVCOLUMN lvc;
				lvc.mask = LVCF_WIDTH;
				RECT rcList;
				GetClientRect(s_hwnd_list, &rcList);
				int cxRemain = rcList.right;
				for (lvc.iSubItem = 0; lvc.iSubItem < colCount; lvc.iSubItem++) {
					lvc.cx = (lvc.iSubItem < nbColSize)
						? MulDiv(e_acxCol[lvc.iSubItem], rcList.right, 100)
						: cxRemain;
					cxRemain -= lvc.cx;
					ListView_SetColumn(s_hwnd_list, lvc.iSubItem, &lvc);
				}
				
				RedrawWindow(hDlg, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_NOFRAME);
				
				WINDOWPLACEMENT wp;
				GetWindowPlacement(hDlg, &wp);
				e_sizeMainDialog.cx = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
				e_sizeMainDialog.cy = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
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
				mii.cch = 0;
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
				for (lvi.iItem = ListView_GetItemCount(s_hwnd_list); --lvi.iItem >= 0;) {
					ListView_GetItem(s_hwnd_list, &lvi);
					Shortcut *const psh = (Shortcut*)lvi.lParam;
					if (psh == pgfi->psh) {
						psh->onGetFileInfo(*pgfi);
					}
				}
				
				delete pgfi;
			}
			break;
	}
	
	return 0;
}


LRESULT CALLBACK prcProgramsTarget(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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
						getWindowProcessName(hWnd, pszPath)) {
					if (rs.isSome()) {
						rs += _T(';');
					}
					rs += pszPath;
				}
				
				String sOldPrograms;
				getDlgItemText(e_hdlgMain, IDCTXT_PROGRAMS, sOldPrograms);
				if (lstrcmp(sOldPrograms, rs)) {
					SetDlgItemText(e_hdlgMain, IDCTXT_PROGRAMS, rs);
				}
			}
			break;
		
		case WM_LBUTTONUP:
			if (s_bCapturingProgram) {
				s_bCapturingProgram = false;
				ReleaseCapture();
				if (s_psh) {
					if (lstrcmp(s_psh->m_sPrograms, s_sOldPrograms)) {
						s_psh->cleanPrograms();
						SetDlgItemText(e_hdlgMain, IDCTXT_PROGRAMS, s_psh->m_sPrograms);
					} else {
						messageBox(hWnd, ERR_TARGET, MB_ICONEXCLAMATION);
					}
				}
			}
			break;
			
		case WM_CAPTURECHANGED:
			if (s_bCapturingProgram) {
				s_bCapturingProgram = false;
				if (s_psh) {
					SetDlgItemText(e_hdlgMain, IDCTXT_PROGRAMS, s_sOldPrograms);
				}
			}
			break;
	}
	
	return CallWindowProc(s_prcProgramsTarget, hWnd, uMsg, wParam, lParam);
}



// Command line shortcut settings
INT_PTR CALLBACK prcCmdSettings(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM MY_UNUSED(lParam)) {
	switch (uMsg) {
		
		case WM_INITDIALOG:
			{
				e_hdlgModal = hDlg;
				centerParent(hDlg);
				
				// Fill "command line show" list
				for (int i = 0; i < shortcut::nbShowOption; i++) {
					SendDlgItemMessage(hDlg, IDCCBO_SHOW, CB_ADDSTRING, 0,
							(LPARAM)getToken(tokShowNormal + i));
				}
				
				SetDlgItemText(hDlg, IDCTXT_COMMAND, s_psh->m_sCommand);
				SetDlgItemText(hDlg, IDCTXT_DIRECTORY, s_psh->m_sDirectory);
				
				int iShow;
				for (iShow = 0; iShow < shortcut::nbShowOption; iShow++) {
					if (shortcut::aiShowOption[iShow] == s_psh->m_nShow) {
						break;
					}
				}
				SendDlgItemMessage(hDlg, IDCCBO_SHOW, CB_SETCURSEL, (WPARAM)iShow, 0);
				
				CheckDlgButton(hDlg, IDCCHK_SUPPORTFILEOPEN, s_psh->m_bSupportFileOpen);
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
						GetDlgItemText(hDlg, IDCTXT_DIRECTORY, pszDir, arrayLength(pszDir));
						
						TCHAR pszTitle[MAX_PATH];
						i18n::loadStringAuto(IDS_BROWSEDIR, pszTitle);
						if (!browseForFolder(hDlg, pszTitle, pszDir)) {
							break;
						}
						
						// Update internal structures
						SetDlgItemText(hDlg, IDCTXT_DIRECTORY, pszDir);
						SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDCTXT_DIRECTORY, EN_KILLFOCUS),
								(LPARAM)GetDlgItem(hDlg, IDCTXT_DIRECTORY));
					}
					break;
				
				case IDOK:
					getDlgItemText(hDlg, IDCTXT_COMMAND, s_psh->m_sCommand);
					getDlgItemText(hDlg, IDCTXT_DIRECTORY, s_psh->m_sDirectory);
					s_psh->m_nShow = shortcut::aiShowOption[
						SendDlgItemMessage(hDlg, IDCCBO_SHOW, CB_GETCURSEL, 0,0)];
					s_psh->m_bSupportFileOpen = toBool(IsDlgButtonChecked(hDlg, IDCCHK_SUPPORTFILEOPEN));
					s_psh->resetIcons();
					// Fall-through
					
				case IDCANCEL:
					e_hdlgModal = e_hdlgMain;
					EndDialog(hDlg, LOWORD(wParam));
					break;
			}
			break;
	}
	
	return 0;
}


// Language selection dialog box
INT_PTR CALLBACK prcLanguage(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM) {
	const HWND hlst = GetDlgItem(hDlg, IDCLST_LANGUAGE);
	
	switch (uMsg) {
		
		case WM_INITDIALOG:
			{
				e_hdlgModal = hDlg;
				
				for (int lang = 0; lang < i18n::langCount; lang++) {
					ListBox_AddString(hlst, getLanguageName(lang));
				}
				ListBox_SetCurSel(hlst, i18n::getLanguage());
			}
			return TRUE;
		
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDCLST_LANGUAGE:
					if (HIWORD(wParam) != LBN_DBLCLK) {
						break;
					}
				// Fall-through
				case IDOK:
					{
						const int iLang = ListBox_GetCurSel(hlst);
						if (i18n::getLanguage() != iLang) {
							i18n::setLanguage(iLang);
							shortcut::saveShortcuts();
						} else {
							wParam = IDCANCEL;
						}
					}
				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
					break;
			}
			break;
	}
	
	return 0;
}



// "About" dialog box
INT_PTR CALLBACK prcAbout(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		
		case WM_INITDIALOG:
			e_hdlgModal = hDlg;
			centerParent(hDlg);
			
			initializeWebLink(hDlg, IDCLBL_WEBSITE, _T("http://utilfr42.free.fr/"));
			initializeWebLink(hDlg, IDCLBL_EMAIL, _T("mailto:guillaume@ryder.fr"));
			i18n::loadStringAuto(IDS_DONATEURL, pszDonateURL);
			initializeWebLink(hDlg, IDCLBL_DONATE, pszDonateURL);
			SendDlgItemMessage(hDlg, IDCLBL_DONATE, STM_SETIMAGE, IMAGE_BITMAP,
				(LPARAM)i18n::loadBitmap(IDB_DONATE));
			return TRUE;
		
		case WM_CTLCOLORSTATIC:
			{
				const int id = GetDlgCtrlID((HWND)lParam);
				if (id == IDCLBL_EMAIL || id == IDCLBL_WEBSITE) {
					HFONT hFont = reinterpret_cast<HFONT>(getWindowLongPtr(hDlg, DWL_USER));
					if (!hFont) {
						LOGFONT lf;
						GetObject((HFONT)SendMessage((HWND)lParam, WM_GETFONT, 0,0), sizeof(lf), &lf);
						lf.lfUnderline = TRUE;
						hFont = CreateFontIndirect(&lf);
						setWindowLongPtr(hDlg, DWL_USER, reinterpret_cast<LONG_PTR>(hFont));
					}
					SelectFont((HDC)wParam, hFont);
					SetTextColor((HDC)wParam, RGB(0,0,255));
					SetBkMode((HDC)wParam, TRANSPARENT);
					return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
				}
			}
			break;
		
		case WM_COMMAND:
			DeleteFont(reinterpret_cast<HFONT>(getWindowLongPtr(hDlg, DWL_USER)));
			EndDialog(hDlg, IDOK);
			break;
	}
	
	return 0;
}


// Redraw the selected item
void updateItem() {
	const int iItem = ListView_GetNextItem(s_hwnd_list, -1, LVNI_SELECTED);
	ListView_RedrawItems(s_hwnd_list, iItem,iItem);
}



// Update the dialog box to reflect the current shortcut state
void updateList() {
	s_bProcessGuiEvents = false;
	
	Shortcut *const psh = getSelectedShortcut();
	const bool bSel = toBool(psh);
	
	HICON hIcon = NULL;
	
	if (bSel) {
		// Fill shortcut controls
		
		s_psh = psh;
		CheckRadioButton(e_hdlgMain, IDCOPT_TEXT, IDCOPT_COMMAND,
			(s_psh->m_bCommand) ? IDCOPT_COMMAND : IDCOPT_TEXT);
		
		SetDlgItemText(e_hdlgMain, IDCTXT_TEXT, s_psh->m_sText);
		SetDlgItemText(e_hdlgMain, IDCTXT_COMMAND, s_psh->m_sCommand);
		SetDlgItemText(e_hdlgMain, IDCTXT_PROGRAMS, s_psh->m_sPrograms);
		SetDlgItemText(e_hdlgMain, IDCTXT_DESCRIPTION, s_psh->m_sDescription);
		SendDlgItemMessage(e_hdlgMain, IDCCBO_PROGRAMS, CB_SETCURSEL,
			s_psh->m_bProgramsOnly, 0);
		
		hIcon = s_psh->getIcon();
	}
	
	
	// Enable or disable shortcut controls and Delete button
	
	const bool bCommand = toBool(IsDlgButtonChecked(e_hdlgMain, IDCOPT_COMMAND));
	static const UINT s_aid[] = {
		IDCCMD_DELETE, IDCCMD_EDIT, IDCFRA_TEXT, IDCFRA_COMMAND,
		IDCOPT_TEXT, IDCOPT_COMMAND,
		IDCLBL_DESCRIPTION, IDCTXT_DESCRIPTION,
		IDCLBL_PROGRAMS, IDCCBO_PROGRAMS, IDCTXT_PROGRAMS, IDCIMG_PROGRAMS,
		IDCTXT_TEXT, IDCCMD_TEXT_MENU,
		IDCTXT_COMMAND, IDCCMD_COMMAND, IDCCMD_CMDSETTINGS, IDCCMD_TEST,
	};
	const bool abEnabled[] = {
		true, true, true, true,
		true, true,
		true, true,
		true, true, true, true,
		!bCommand, !bCommand,
		bCommand, bCommand, bCommand, bCommand,
	};
	for (int i = 0; i < arrayLength(s_aid); i++) {
		EnableWindow(GetDlgItem(e_hdlgMain, s_aid[i]), bSel && abEnabled[i]);
	}
	
	
	// Icon
	SendDlgItemMessage(e_hdlgMain, IDCLBL_ICON, STM_SETICON,
		(WPARAM)((bCommand) ? hIcon : NULL), 0);
	
	s_bProcessGuiEvents = true;
}


// Insert a new item in the shortcut list
// Return its ID
int addItem(Shortcut* psh, bool bSelected) {
	LVITEM lvi;
	lvi.iItem = 0;
	lvi.iSubItem = 0;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	lvi.state = (bSelected) ? LVIS_SELECTED | LVIS_FOCUSED : 0;
	lvi.lParam = (LPARAM)psh;
	lvi.iImage = I_IMAGECALLBACK;
	lvi.iItem = ListView_InsertItem(s_hwnd_list, &lvi);
	lvi.mask = LVIF_TEXT;
	for (lvi.iSubItem = 1; lvi.iSubItem < arrayLength(e_acxCol); lvi.iSubItem++) {
		ListView_SetItem(s_hwnd_list, &lvi);
	}
	
	return lvi.iItem;
}


// Called when some columns of an item are updated.
void onItemUpdated(int mskColumns) {
	if (mskColumns & (1 << Shortcut::s_iSortColumn)) {
		ListView_SortItems(s_hwnd_list, Shortcut::compare, NULL);
	}
	ListView_EnsureVisible(s_hwnd_list, ListView_GetSelectionMark(s_hwnd_list), FALSE);
}


// Called when a shortcut is added or removed.
// Updates the shortcut counts label.
void onShortcutsCountChanged() {
	SetDlgItemInt(e_hdlgModal, IDCTXT_SHORTCUTSCOUNT,
		(UINT)ListView_GetItemCount(s_hwnd_list), TRUE);
}


// Get the selected shortcut
Shortcut* getSelectedShortcut() {
	LVITEM lvi;
	lvi.iItem = ListView_GetNextItem(s_hwnd_list, -1, LVNI_SELECTED);
	if (lvi.iItem < 0) {
		return NULL;
	}
	lvi.mask = LVIF_PARAM;
	ListView_GetItem(s_hwnd_list, &lvi);
	return (Shortcut*)lvi.lParam;
}

}  // dialogs namespace


//------------------------------------------------------------------------
// Shortcut
//------------------------------------------------------------------------

namespace shortcut {

// Get several icons, then delete the GETFILEICON* array.
DWORD WINAPI threadGetFilesIcon(GETFILEICON* apgfi[]) {
	for (int i = 0; apgfi[i]; i++) {
		threadGetFileIcon(*apgfi[i]);
	}
	delete [] apgfi;
	return 0;
}


// Get an icon, then give it to the main window by posting a WM_GETFILEICON message.
DWORD WINAPI threadGetFileIcon(GETFILEICON& gfi) {
	// Use HTML files icon for URLs
	if (PathIsURL(gfi.pszExecutable)) {
		lstrcpy(gfi.pszExecutable, _T("a.url"));
		gfi.uFlags |= SHGFI_USEFILEATTRIBUTES;
	}
	
	gfi.bOK = getFileInfo(gfi.pszExecutable, 0, gfi.shfi, gfi.uFlags);
	
	PostMessage(dialogs::e_hdlgMain, WM_GETFILEICON, 0, (LPARAM)&gfi);
	return 0;
}


void Shortcut::findExecutable(LPTSTR pszExecutable) {
	TCHAR pszFile[MAX_PATH];
	StrCpyN(pszFile, m_sCommand, arrayLength(pszFile));
	PathRemoveArgs(pszFile);
	findFullPath(pszFile, pszExecutable);
}


void Shortcut::onGetFileInfo(GETFILEICON& gfi) {
	if (gfi.uFlags & SHGFI_SYSICONINDEX) {
		// Small icon index
		
		m_iSmallIcon = (gfi.bOK) ? gfi.shfi.iIcon : iconInvalid;
		
		LVFINDINFO lvfi;
		lvfi.flags = LVFI_PARAM;
		lvfi.lParam = (LPARAM)gfi.psh;
		const int iItem = ListView_FindItem(dialogs::s_hwnd_list, -1, &lvfi);
		if (iItem >= 0) {
			ListView_RedrawItems(dialogs::s_hwnd_list, iItem,iItem);
		}
		
	} else {
		// Big icon
		
		m_hIcon = (gfi.bOK) ? gfi.shfi.hIcon : NULL;
		if (dialogs::s_psh == this) {
			SendDlgItemMessage(dialogs::e_hdlgMain, IDCLBL_ICON, STM_SETICON,
					(WPARAM)((m_bCommand) ? m_hIcon : NULL), 0);
		}
	}
}

void Shortcut::findSmallIconIndex() {
	VERIFV(m_iSmallIcon != iconThreadRunning);
	m_iSmallIcon = iconThreadRunning;
	
	if (m_bCommand) {
		fillGetFileIcon(NULL, true);
	}
}

void Shortcut::fillGetFileIcon(GETFILEICON* pgfi, bool bSmallIcon) {
	const bool bStart = !pgfi;
	if (bStart) {
		pgfi = new GETFILEICON;
	}
	
	if (bSmallIcon) {
		m_iSmallIcon = iconThreadRunning;
	}
	
	pgfi->psh = this;
	pgfi->uFlags = (bSmallIcon)
		? SHGFI_ICON | SHGFI_SMALLICON | SHGFI_SYSICONINDEX
		: SHGFI_ICON;
	findExecutable(pgfi->pszExecutable);
	
	if (bStart) {
		if (*pgfi->pszExecutable) {
			startThread((LPTHREAD_START_ROUTINE)threadGetFileIcon, pgfi);
		} else {
			threadGetFileIcon(*pgfi);
		}
	}
}

void Shortcut::findIcon() {
	fillGetFileIcon(NULL, false);
}

int Shortcut::getSmallIconIndex() {
	if (!m_bCommand) {
		return -1;
	}
	if (m_iSmallIcon == iconNeeded) {
		findSmallIconIndex();
	}
	return m_iSmallIcon;
}

void Shortcut::resetIcons() {
	m_iSmallIcon = iconNeeded;
	if (m_hIcon) {
		DestroyIcon(m_hIcon);
		m_hIcon = NULL;
	}
}


// Add shortcut tabular representation to the given string
// Return the new end of the string
void Shortcut::appendCsvLineToString(String& rs) const {
	TCHAR pszHotKey[bufHotKey];
	getKeyName(pszHotKey);
	rs += pszHotKey;
	rs += _T('\t');
	
	rs += getToken((m_bCommand) ? tokCommand : tokText);
	rs += _T('\t');
	
	rs += (m_bCommand) ? m_sCommand : m_sText;
	rs += _T('\t');
	
	String sCond;
	getColumnText(colCond, sCond);
	rs += sCond;
	rs += _T('\t');
	
	rs += m_sDescription;
	rs += _T("\r\n");
}


void Shortcut::getColumnText(int iColumn, String& rs) const {
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
				
				for (int cond = 0; cond < 3; cond++) {
					if (m_aCond[cond]) {
						rs += s_acCond[m_aCond[cond]];
						rs += s_sCondKeys.get()[cond];
					}
				}
				if (m_sPrograms.isSome()) {
					if (rs.isSome()) {
						rs += _T(' ');
					}
					rs += (m_bProgramsOnly) ? _T('+') : _T('-');
					rs += m_sPrograms;
				}
			}
			break;
		
		case colDescription:
			rs = m_sDescription;
			break;
	}
}


int Shortcut::s_iSortColumn = colContents;

// Compare two shortcuts by shortcut name
int CALLBACK Shortcut::compare(const Shortcut* psh1, const Shortcut* psh2, LPARAM) {
	switch (s_iSortColumn) {
		case colContents:
			if (psh1->m_bCommand != psh2->m_bCommand) {
				return (!psh1->m_bCommand || psh2->m_bCommand) ? +1 : -1;
			}
			break;
		
		case colShortcut:
			{
				const int vkFlags1 = psh1->m_vkFlags;
				const int vkFlags2 = psh2->m_vkFlags;
				if (vkFlags1 != vkFlags2) {
					for (int i = 0; i < arrayLength(e_aSpecialKey); i++) {
						int vkFlags = e_aSpecialKey[i].vkFlags;
						int diff = (int)(vkFlags1 & vkFlags) - (int)(vkFlags2 & vkFlags);
						if (diff) {
							return diff;
						}
						vkFlags <<= vkFlagsRightOffset;
						diff = (int)(vkFlags1 & vkFlags) - (int)(vkFlags2 & vkFlags);
						if (diff) {
							return diff;
						}
					}
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

}  // shortcut namespace


namespace dialogs {

void fillSpecialCharsMenu(HMENU hMenu, int pos, UINT idFirst) {
	const HMENU hSubMenu = GetSubMenu(hMenu, pos);
	RemoveMenu(hSubMenu, 0, MF_BYPOSITION);
	for (int chr = 128; chr < 256; chr++) {
		TCHAR pszLabel[30];
		wsprintf(pszLabel, _T("%d - %c"), chr, chr);
		AppendMenu(hSubMenu,
			((chr & 15) == 0) ? MF_STRING | MF_MENUBREAK : MF_STRING,
			idFirst + chr, pszLabel);
	}
}

void escapeString(LPCTSTR psz, String& rs) {
	rs.getBuffer(lstrlen(psz) + 1);
	for (;;) {
		switch (*psz) {
			case '\0':
				return;
			case '\\':
			case '[':
			case ']':
			case '{':
			case '}':
			case '|':
				rs += _T('\\');
			default:
				rs += *psz++;
				break;
		}
	}
}

void appendText(LPCTSTR pszText) {
	const HWND hctl = GetDlgItem(e_hdlgMain, IDCTXT_TEXT);
	Edit_ReplaceSel(hctl, pszText);
	SetFocus(hctl);
}

}  // dialogs namespace
