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
#include "I18n.h"
#include "Shortcut.h"

#ifndef OPENFILENAME_SIZE_VERSION_400
#define OPENFILENAME_SIZE_VERSION_400  sizeof(OPENFILENAME)
#endif


namespace dialogs {

struct GETFILEICON {
	Shortcut* shortcut;
	SHFILEINFO shfi;
	TCHAR executable[MAX_PATH];
	UINT flags;
	bool ok;
};

namespace {

TranslatedString s_cond_key_translations;

Shortcut *s_shortcut = nullptr;  // Shortcut being edited

HWND s_hwnd_list = NULL;  // Shortcuts list

// Process GUI events to update shortcuts data from the dialog box controls
bool s_process_gui_events;

constexpr LPCTSTR kHelpUrlFormat = _T("https://gryder.org/software/clavier-plus/documentation?lang=%s");
TCHAR s_donate_url[256];

// Main dialog box.
INT_PTR CALLBACK prcMain(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);

// Command line shortcut settings dialog bux.
INT_PTR CALLBACK prcCmdSettings(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);

// Language selection dialog box.
INT_PTR CALLBACK prcLanguage(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);

// "About" dialog box.
INT_PTR CALLBACK prcAbout(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);

// s_hwnd_list procedure.
LRESULT CALLBACK prcList(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR subclass_id, DWORD_PTR ref_data);

// IDCIMG_PROGRAMS procedure.
LRESULT CALLBACK prcProgramsTarget(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR subclass_id, DWORD_PTR ref_data);

// ID of the next program or app menu item to add to the "Add" sub-menus.
UINT s_add_menu_next_id;

bool browseForCommandLine(HWND hwnd_parent, LPTSTR file, bool force_exist);
bool browseForCommandLine(HWND hdlg);
void onMainCommand(UINT id, WORD notify, HWND hwnd);

Shortcut* getSelectedShortcut();

// Redraws the selected item.
void updateItem();

// Update the dialog box to reflect the current shortcut state
void updateList();

// Adds a new item to the shortcut list. Returns its ID.
int addItem(Shortcut* shortcut, bool selected);

// Called when some columns of an item are updated.
void onItemUpdated(int columns_mask);

// Called when a shortcut is added or removed.
// Updates the shortcut counts label.
void onShortcutsCountChanged();

void fillSpecialCharsMenu(HMENU menu, int pos, UINT first_id);

// Appends the escaped version of the input string to the output.
void escapeString(LPCTSTR input, String* output);

// Appends text to the current shortcut text in the dialog bux.
void appendText(LPCTSTR text);

// Get an icon, then give it to the main window by posting a WM_GETFILEICON message.
DWORD WINAPI threadGetFileIcon(dialogs::GETFILEICON& gfi);

// Get several icons, then delete the GETFILEICON* array.
DWORD WINAPI threadGetFilesIcon(dialogs::GETFILEICON* apgfi[]);

}  // namespace

HWND e_hdlgMain;


void initializeCurrentLanguage() {
	s_cond_key_translations.load(IDS_CONDITION_KEYS);
}


INT_PTR showMainDialogModal(UINT initial_command) {
	return i18n::dialogBox(IDD_MAIN, /* hwnd_parent= */ NULL, prcMain, initial_command);
}

namespace {

// For PLACEMENT::flags.
constexpr int kPlacementLeft = 1 << 0;
constexpr int kPlacementLeftHalf = 1 << 1;
constexpr int kPlacementWidth = 1 << 2;
constexpr int kPlacementWidthHalf = 1 << 3;
constexpr int kPlacementTop = 1 << 4;
constexpr int kPlacementHeight = 1 << 5;

struct PLACEMENT {
	int id;
	int flags;
	
	HWND hctl;
	POINT point;  // Top-left corner.
	SIZE size;
	
	
	void updateGui(int dx, int dy) {
		POINT new_point = point;
		SIZE new_size = size;
		
		if (flags & kPlacementLeft) {
			new_point.x += dx;
		} else if (flags & kPlacementLeftHalf) {
			new_point.x += dx / 2;
		}
		
		if (flags & kPlacementWidth) {
			new_size.cx += dx;
		} else if (flags & kPlacementWidthHalf) {
			new_size.cx += dx / 2;
		}
		
		if (flags & kPlacementTop) {
			new_point.y += dy;
		}
		
		if (flags & kPlacementHeight) {
			new_size.cy += dy;
		}
		
		MoveWindow(hctl, new_point.x, new_point.y, new_size.cx, new_size.cy, /* bRepaint= */ false);
	}
};

PLACEMENT main_dialog_placements[] = {
	{ IDCLST, kPlacementWidth | kPlacementHeight },
	
	{ IDCCMD_ADD, kPlacementTop },
	{ IDCCMD_DELETE, kPlacementTop },
	{ IDCCMD_EDIT, kPlacementTop },
	{ IDCLBL_HELP, kPlacementTop },
	{ IDCLBL_SHORTCUTSCOUNT, kPlacementTop },
	{ IDCTXT_SHORTCUTSCOUNT, kPlacementTop },
	{ IDCCHK_AUTOSTART, kPlacementTop },
	
	{ IDCOPT_TEXT, kPlacementTop },
	{ IDCFRA_TEXT, kPlacementTop | kPlacementWidthHalf },
	{ IDCTXT_TEXT, kPlacementTop | kPlacementWidthHalf },
	{ IDCCMD_TEXT_MENU, kPlacementTop | kPlacementLeftHalf },
	
	{ IDCOPT_COMMAND, kPlacementTop | kPlacementLeftHalf },
	{ IDCFRA_COMMAND, kPlacementTop | kPlacementLeftHalf | kPlacementWidthHalf },
	{ IDCTXT_COMMAND, kPlacementTop | kPlacementLeftHalf | kPlacementWidthHalf },
	{ IDCCMD_COMMAND, kPlacementTop | kPlacementLeft },
	{ IDCLBL_ICON, kPlacementTop | kPlacementLeftHalf },
	{ IDCCMD_CMDSETTINGS, kPlacementTop | kPlacementLeftHalf },
	{ IDCCMD_TEST, kPlacementTop | kPlacementLeftHalf },
	
	{ IDCLBL_PROGRAMS, kPlacementTop },
	{ IDCCBO_PROGRAMS, kPlacementTop },
	{ IDCTXT_PROGRAMS, kPlacementTop | kPlacementWidth },
	{ IDCIMG_PROGRAMS, kPlacementTop | kPlacementLeft },
	
	{ IDCLBL_DESCRIPTION, kPlacementTop },
	{ IDCTXT_DESCRIPTION, kPlacementTop | kPlacementWidth },
	
	{ IDCLBL_DONATE, kPlacementTop },
	
	{ IDCCMD_HELP, kPlacementTop | kPlacementLeft },
	{ IDCCMD_COPYLIST, kPlacementTop | kPlacementLeft },
	{ IDCCMD_LANGUAGE, kPlacementTop | kPlacementLeft },
	{ IDCCMD_ABOUT, kPlacementTop | kPlacementLeft },
	{ IDCCMD_QUIT, kPlacementTop | kPlacementLeft },
	{ IDCANCEL, kPlacementTop | kPlacementLeft },
	{ IDOK, kPlacementTop | kPlacementLeft },
};


// Ask the user to enter a keystroke for the shortcut.
// Add the shortcut on success, else delete it.
// Takes ownership of the shortcut.
void addShortcut(Shortcut* shortcut) {
	// Ask the user for the keystroke.
	SetFocus(s_hwnd_list);
	if (!shortcut->showEditDialog(dialogs::e_hdlgMain)) {
		delete shortcut;
		return;
	}
	
	// Add the shortcut to the list, in selected state.
	s_process_gui_events = false;
	addItem(shortcut, /* selected= */ true);
	onItemUpdated(~0);
	updateList();
	onShortcutsCountChanged();
	s_process_gui_events = true;
	
	// Give the focus to the main control of the shortcut.
	const HWND edit_control = GetDlgItem(
		e_hdlgMain, (shortcut->m_type == Shortcut::Type::kCommand) ? IDCTXT_COMMAND : IDCTXT_TEXT);
	if (shortcut->m_type == Shortcut::Type::kCommand) {
		const int len = shortcut->m_command.getLength();
		Edit_SetSel(edit_control, len, len);
	}
	SetFocus(edit_control);
}

// Create a new command shortcut for the given path, using resolveLinkFile().
// Ask the user to enter a keystroke before adding the shortcut.
void addPathShortcut(LPCTSTR path) {
	Shortcut *const shortcut = new Shortcut;
	resolveLinkFile(path, shortcut);
	addShortcut(shortcut);
}

// Create a new command shortcut for the given command-line.
// Ask the user to enter a keystroke before adding the shortcut.
void addCommandShortcut(LPCTSTR command, bool support_file_open = false) {
	Shortcut *const shortcut = new Shortcut;
	shortcut->m_type = Shortcut::Type::kCommand;
	shortcut->m_command = command;
	shortcut->m_support_file_open = support_file_open;
	addShortcut(shortcut);
}


// Base class for dynamic entries in "Add" sub-menus.
class BaseMenuItem {
public:
	
	static BaseMenuItem* forItemData(ULONG_PTR item_data) {
		return reinterpret_cast<BaseMenuItem*>(item_data);
	}
	
	ULONG_PTR asItemData() const {
		return reinterpret_cast<ULONG_PTR>(this);
	}
	
	virtual ~BaseMenuItem() = default;
	
	void initializeRoot(HMENU menu, int item_index);
	
	static BaseMenuItem* findItemAndCleanMenu(HMENU menu, UINT id);
	
	void measureItem(MEASUREITEMSTRUCT* mis);
	void drawItem(const DRAWITEMSTRUCT& dis);
	virtual void populate(HMENU menu) = 0;
	virtual void execute() = 0;
	
protected:
	
	BaseMenuItem(int icon_index) : m_icon_index(icon_index) {}
	
	BaseMenuItem(const BaseMenuItem& other) = delete;
	BaseMenuItem& operator =(const BaseMenuItem& other) = delete;
	
private:
	
	int m_icon_index;
	
	static constexpr int kIconMarginPixel = 3;
};

HIMAGELIST s_sys_imagelist = NULL;

void BaseMenuItem::initializeRoot(HMENU menu, int item_index) {
	// Attach this item to the sub-menu.
	MENUITEMINFO mii_root;
	mii_root.cbSize = sizeof(mii_root);
	mii_root.fMask = MIIM_DATA;
	mii_root.dwItemData = asItemData();
	SetMenuItemInfo(menu, item_index, /* byPosition= */ true, &mii_root);
	
	// Add a phantom item to the sub-menu.
	MENUITEMINFO mii_phantom;
	mii_phantom.cbSize = sizeof(mii_phantom);
	mii_phantom.fMask = MIIM_TYPE | MIIM_DATA;
	mii_phantom.fType = MFT_SEPARATOR;
	mii_phantom.dwTypeData = _T("");
	mii_phantom.dwItemData = asItemData();
	SetMenuItemInfo(GetSubMenu(menu, item_index), 0, /* byPosition= */ true, &mii_phantom);
}

void BaseMenuItem::measureItem(MEASUREITEMSTRUCT* mis) {
	mis->itemWidth = 2 * kIconMarginPixel;
	mis->itemHeight = 2 * kIconMarginPixel + GetSystemMetrics(SM_CYSMICON);
}

void BaseMenuItem::drawItem(const DRAWITEMSTRUCT& dis) {
	ImageList_Draw(
		s_sys_imagelist, m_icon_index, dis.hDC,
		dis.rcItem.left - GetSystemMetrics(SM_CXSMICON),
		(dis.rcItem.top + dis.rcItem.bottom - GetSystemMetrics(SM_CYSMICON)) / 2,
		ILD_TRANSPARENT);
}


BaseMenuItem* BaseMenuItem::findItemAndCleanMenu(HMENU menu, UINT id) {
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_ID | MIIM_SUBMENU;
	
	BaseMenuItem *item_to_execute = nullptr;
	for (UINT pos = 0; GetMenuItemInfo(menu, pos, /* fByPosition= */ true, &mii); pos++) {
		BaseMenuItem *item = reinterpret_cast<BaseMenuItem*>(mii.dwItemData);
		if (item) {
			if (mii.wID == id && id) {
				item_to_execute = item;
			} else if (!(mii.fType & MFT_SEPARATOR)) {
				delete item;
			}
		}
		if (mii.hSubMenu) {
			item = findItemAndCleanMenu(mii.hSubMenu, id);
			if (item && !item_to_execute) {
				item_to_execute = item;
			}
		}
	}
	
	return item_to_execute;
}


// File or directory entry in the "Add > Program" sub-menu.
// Also instantiated with dummy values for the sub-menu item itself.
class FileMenuItem : public BaseMenuItem {
public:
	
	FileMenuItem(bool is_dir, LPCTSTR path, int icon_index)
		: BaseMenuItem(icon_index), m_is_dir(is_dir), m_path(path) {}
	
	bool isDir() const { return m_is_dir; }
	void populate(HMENU menu) override;
	void execute() override;
	
private:
	
	bool m_is_dir;
	String m_path;
};

void FileMenuItem::populate(HMENU menu) {
	// For retrieving item data.
	MENUITEMINFO mii_get_data;
	mii_get_data.cbSize = sizeof(mii_get_data);
	mii_get_data.fMask = MIIM_DATA;
	
	// For inserting real items.
	MENUITEMINFO mii_real;
	mii_real.cbSize = sizeof(mii_real);
	mii_real.fMask = MIIM_BITMAP | MIIM_STRING;
	mii_real.hSubMenu = NULL;
	
	// For inserting phantom items.
	MENUITEMINFO mii_phantom;
	mii_phantom.cbSize = sizeof(mii_phantom);
	mii_phantom.fMask = MIIM_TYPE | MIIM_DATA;
	mii_phantom.fType = MFT_SEPARATOR;
	mii_phantom.dwTypeData = _T("");
	
	UINT pos_folder = 0;
	
	// Two passes: 1) all-users start menu; 2) user start menu.
	for (int pass_index = 0; pass_index < 2; pass_index++) {
		TCHAR folder[MAX_PATH];
		if (!getSpecialFolderPath(pass_index ? CSIDL_COMMON_STARTMENU : CSIDL_STARTMENU, folder)) {
			continue;
		}
		if (m_path.isSome()) {
			PathAppend(folder, m_path);
		}
		
		// Enumerate files in the directory.
		TCHAR path[MAX_PATH];
		PathCombine(path, folder, _T("*"));
		WIN32_FIND_DATA wfd;
		const HANDLE hff = FindFirstFile(path, &wfd);
		if (hff == INVALID_HANDLE_VALUE) {
			continue;
		}
		do {
			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) {
				continue;
			}
			PathCombine(path, folder, wfd.cFileName);
			
			TCHAR relative_folder[MAX_PATH];
			LPTSTR item_path;
			
			const bool is_dir = toBool(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
			
			UINT pos;
			if (is_dir) {
				// Directory: skip "." and ".." directories.
				if (wfd.cFileName[0] == _T('.') &&
						(wfd.cFileName[1] == _T('.') || !wfd.cFileName[1])) {
					continue;
				}
				
				lstrcpyn(relative_folder, m_path, arrayLength(relative_folder));
				PathAppend(relative_folder, wfd.cFileName);
				item_path = relative_folder;
				
				if (pass_index) {
					// Second pass: the first pass may have already created a menu for the directory.
					const int item_count = GetMenuItemCount(menu);
					for (int i = 0; i < item_count; i++) {
						if (!GetMenuItemInfo(menu, i, /* fByPosition= */ true, &mii_get_data)) {
							continue;
						}
						const FileMenuItem *fmi_other =
							static_cast<const FileMenuItem*>(BaseMenuItem::forItemData(mii_get_data.dwItemData));
						if (fmi_other->isDir() && !lstrcmpi(fmi_other->m_path, relative_folder)) {
							// Folder already created: nothing to do.
							goto Next;
						}
					}
				}
				mii_real.fMask = MIIM_BITMAP | MIIM_DATA | MIIM_STRING | MIIM_SUBMENU;
				mii_real.hSubMenu = CreatePopupMenu();
				pos = pos_folder++;
			} else {
				// File.
				mii_real.fMask = MIIM_BITMAP | MIIM_DATA | MIIM_STRING | MIIM_ID;
				mii_real.wID = s_add_menu_next_id++;
				pos = (UINT)-1;
				PathRemoveExtension(wfd.cFileName);
				item_path = path;
			}
			
			// Get the small icon
			SHFILEINFO shfi;
			if (!getFileInfo(path, wfd.dwFileAttributes, shfi,
					SHGFI_ICON | SHGFI_SMALLICON | SHGFI_SYSICONINDEX)) {
				shfi.iIcon = -1;
			}
			
			mii_real.dwItemData = (new FileMenuItem(is_dir, item_path, shfi.iIcon))->asItemData();
			mii_real.dwTypeData = wfd.cFileName;
			mii_real.hbmpItem = HBMMENU_CALLBACK;
			
			// Create the phantom sub-item used by WM_INITMENUPOPUP
			// to get the parent of the popup to initialize
			if (is_dir) {
				mii_phantom.dwItemData = mii_real.dwItemData;
				InsertMenuItem(mii_real.hSubMenu, 0, /* fByPosition= */ true, &mii_phantom);
			}
			
			InsertMenuItem(menu, pos, /* fByPosition= */ true, &mii_real);
			
		Next:
			;
		} while (FindNextFile(hff, &wfd));
		FindClose(hff);
	}
	
	// Insert menu bar breaks every N items where N = screen height / item height.
	const int nb_item_per_column =
		(GetSystemMetrics(SM_CYSCREEN) - 10) / GetSystemMetrics(SM_CYMENU);
	MENUITEMINFO mii_menu_bar_break;
	mii_menu_bar_break.cbSize = sizeof(mii_get_data);
	mii_menu_bar_break.fMask = MIIM_TYPE;
	int item_count = GetMenuItemCount(menu);
	for (int i = nb_item_per_column; i < item_count; i += nb_item_per_column) {
		GetMenuItemInfo(menu, i, /* fByPosition= */ true, &mii_menu_bar_break);
		mii_menu_bar_break.fType |= MFT_MENUBARBREAK;
		SetMenuItemInfo(menu, i, /* fByPosition= */ true, &mii_menu_bar_break);
	}
}

void FileMenuItem::execute() {
	addPathShortcut(m_path);
}


// Entry in the "Add > Application" sub-menu.
// Also instantiated with dummy values for the sub-menu item itself.
class AppMenuItem : public BaseMenuItem {
public:
	
	AppMenuItem(LPCTSTR app_id, int icon_index)
		: BaseMenuItem(icon_index), m_app_id(app_id) {}
	
	void populate(HMENU menu) override;
	void execute() override;
	
private:
	
	String m_app_id;
};

void AppMenuItem::populate(HMENU menu) {
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_BITMAP | MIIM_DATA | MIIM_STRING | MIIM_ID;
	mii.hbmpItem = HBMMENU_CALLBACK;
	
	int index = 0;
	listUwpApps(
		[&, menu](LPCTSTR name, LPCTSTR app_id, LPITEMIDLIST pidl) {
			// Retrieve the app's icon.
			SHFILEINFO shfi;
			if (!SHGetFileInfo(
					reinterpret_cast<LPCTSTR>(pidl),
					/* dwFileAttributes= */ 0,
					&shfi, sizeof(shfi),
					SHGFI_PIDL | SHGFI_ICON | SHGFI_SMALLICON | SHGFI_SYSICONINDEX)) {
				shfi.iIcon =-1;
			}
			
			mii.wID = s_add_menu_next_id++;
			mii.dwItemData = (new AppMenuItem(app_id, shfi.iIcon))->asItemData();
			mii.dwTypeData = const_cast<LPTSTR>(name);
			InsertMenuItem(menu, index++, /* byPosition= */ true, &mii);
		});
}

void AppMenuItem::execute() {
	String path = _T("explorer.exe shell:appsFolder\\");
	path += m_app_id;
	addCommandShortcut(path);
}


bool browseForCommandLine(HWND hwnd_parent, LPTSTR file, bool force_exist) {
	PathUnquoteSpaces(file);
	
	// Ask for the file
	OPENFILENAME ofn;
	ZeroMemory(&ofn, OPENFILENAME_SIZE_VERSION_400);
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner = hwnd_parent;
	ofn.lpstrFile = file;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = force_exist ? OFN_FILEMUSTEXIST | OFN_HIDEREADONLY : OFN_HIDEREADONLY;
	VERIF(GetOpenFileName(&ofn));
	
	PathQuoteSpaces(file);
	return true;
}


bool browseForCommandLine(HWND hdlg) {
	TCHAR file[MAX_PATH];
	GetDlgItemText(hdlg, IDCTXT_COMMAND, file, arrayLength(file));
	PathRemoveArgs(file);
	
	VERIF(browseForCommandLine(hdlg, file, /* force_exist= */ true));
	
	SetDlgItemText(hdlg, IDCTXT_COMMAND, file);
	return true;
}


bool s_capturing_program = false;
String s_old_programs;

// Indexed by (command_id - ID_TEXT_LOWLEVEL).
constexpr LPCTSTR kWriteCommands[] = {
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
	_T("[{MouseWheel,<(-) ticks>}]"),
	_T("[{KeysDown,<keys>}]"),
	_T("[{FocusOrLaunch,<window name>,<command>,<delay>}]"),
};


void onMainCommand(UINT id, WORD notify, HWND hwnd) {
	switch (id) {
		
		case IDCANCEL:
			{
				// Get delete new shortcuts
				LVITEM lvi;
				lvi.mask = LVIF_PARAM;
				for (lvi.iItem = 0; ListView_GetItem(s_hwnd_list, &lvi); lvi.iItem++) {
					delete reinterpret_cast<Shortcut*>(lvi.lParam);
				}
				ListView_DeleteAllItems(s_hwnd_list);
				
				for (Shortcut* sh = shortcut::getFirst(); sh; sh = sh->getNext()) {
					sh->registerHotKey();
				}
				
				EndDialog(e_hdlgMain, IDCANCEL);
			}
			break;
		
		case IDCCMD_QUIT:
		case IDOK:
		case IDCCMD_LANGUAGE:
			{
				// Test for cancellation before doing anything else.
				if (id == IDCCMD_LANGUAGE) {
					if (i18n::dialogBox(IDD_LANGUAGE, e_modal_dialog, prcLanguage) == IDCANCEL) {
						break;
					}
				} else if (id == IDCCMD_QUIT) {
					if (!isKeyDown(VK_SHIFT) && IDNO ==
							messageBox(e_hdlgMain, ASK_QUIT, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION)) {
						break;
					}
				}
				
				// Get all shortcuts from the list, check unicity
				const int item_count = ListView_GetItemCount(s_hwnd_list);
				Shortcut **const shortcuts = new Shortcut*[item_count];
				LVITEM lvi;
				lvi.mask = LVIF_PARAM;
				for (lvi.iItem = 0; lvi.iItem < item_count; lvi.iItem++) {
					ListView_GetItem(s_hwnd_list, &lvi);
					Shortcut *const shortcut = shortcuts[lvi.iItem] = reinterpret_cast<Shortcut*>(lvi.lParam);
					String *const programs = shortcut->getPrograms();
					for (int i = 0; i < lvi.iItem; i++) {
						if (shortcuts[i]->testConflict(*shortcut, programs, shortcut->m_programs_only)) {
							TCHAR shortcut_display_name[kHotKeyBufSize];
							shortcut->getDisplayName(shortcut_display_name);
							messageBox(e_hdlgMain, ERR_SHORTCUT_DUPLICATE, MB_ICONERROR, shortcut_display_name);
							delete [] programs;
							delete [] shortcuts;
							return;
						}
					}
					delete [] programs;
				}
				
				// Create a valid linked list from the list box
				// and register the hot keys
				shortcut::clearShortcuts();
				for (int i = 0; i < item_count; i++) {
					Shortcut *const shortcut = shortcuts[i];
					shortcut->clearIcons();
					shortcut->registerHotKey();
					shortcut->addToList();
				}
				delete [] shortcuts;
				
				if (id != IDCCMD_QUIT) {
					shortcut::saveShortcuts();
				}
				
				setAutoStartEnabled(toBool(IsDlgButtonChecked(e_hdlgMain, IDCCHK_AUTOSTART)));
			}
			
			EndDialog(e_hdlgMain, id);
			break;
		
		// Display "Add" menu
		case IDCCMD_ADD:
			{
				const HMENU context_menus = i18n::loadMenu(IDM_CONTEXT);
				
				HMENU menu = GetSubMenu(context_menus, 0);
				
				s_add_menu_next_id = ID_ADD_ITEM_FIRST;
				int item_index = 0;
				
				// Programs sub-menu.
				(new FileMenuItem(/* is_dir= */ true, /* path= */ nullptr, /* icon_index= */ 0))
					->initializeRoot(menu, item_index++);
				
				// Apps sub-menu.
				(new AppMenuItem(/* app_id= */ _T(""), /* icon_index= */ 0))
					->initializeRoot(menu, item_index++);
				
				// Special characters sub-menu.
				fillSpecialCharsMenu(menu, item_index, ID_ADD_SPECIALCHAR_FIRST);
				
				// Show the context menu
				RECT dialog_rect;
				GetWindowRect(hwnd, &dialog_rect);
				const UINT selected_cmd_id = static_cast<UINT>(TrackPopupMenu(
					menu,
					TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD,
					dialog_rect.left, dialog_rect.bottom, 0, e_hdlgMain, /* prcRect= */ nullptr));
				BaseMenuItem *const item = BaseMenuItem::findItemAndCleanMenu(menu, selected_cmd_id);
				DestroyMenu(context_menus);
				
				if (item) {
					item->execute();
					delete item;
				} else if (selected_cmd_id) {
					SendMessage(e_hdlgMain, WM_COMMAND, selected_cmd_id, 0);
				}
			}
			break;
		
		case ID_ADD_FOLDER:
			{
				TCHAR folder[MAX_PATH] = _T("");
				if (browseForFolder(e_hdlgMain, /* title= */ nullptr, folder)) {
					PathQuoteSpaces(folder);
					addCommandShortcut(folder, /* support_file_open= */ true);
				}
			}
			break;
		
		case ID_ADD_TEXT:
			addShortcut(new Shortcut);
			break;
		
		case ID_ADD_COMMAND:
			addCommandShortcut(_T(""));
			break;
		
		case ID_ADD_WEBSITE:
			addCommandShortcut(_T("https://"));
			break;
		
		
		case IDCCMD_DELETE:
			{
				// Retrieve the selected list item, if any.
				LVITEM lvi;
				lvi.iItem = ListView_GetNextItem(s_hwnd_list, -1, LVNI_SELECTED);
				if (lvi.iItem < 0) {
					// No item selected. Can happen if VK_DELETE triggered IDCCMD_DELETE.
					break;
				}
				
				if (!isKeyDown(VK_SHIFT) && IDNO == messageBox(
						e_hdlgMain, ASK_DELETE, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION)) {
					break;
				}
				
				SetFocus(GetDlgItem(e_hdlgMain, IDCCMD_ADD));
				SendMessage(e_hdlgMain, WM_NEXTDLGCTL, /* previous= */ true, /* absolute= */ false);
				
				// Remove the item from the list.
				lvi.mask = LVIF_PARAM;
				ListView_GetItem(s_hwnd_list, &lvi);
				
				// Delete item, hotkey, and shortcut object
				ListView_DeleteItem(s_hwnd_list, lvi.iItem);
				Shortcut *const sh = reinterpret_cast<Shortcut*>(lvi.lParam);
				sh->unregisterHotKey();
				delete sh;
				
				// Select an other item and update
				if (lvi.iItem >= ListView_GetItemCount(s_hwnd_list)) {
					lvi.iItem--;
				}
				ListView_SetItemState(
					s_hwnd_list, lvi.iItem,
					LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
				updateList();
				onShortcutsCountChanged();
				SetFocus(s_hwnd_list);
			}
			break;
		
		case IDCCMD_EDIT:
			SetFocus(s_hwnd_list);
			{
				Keystroke ks = *s_shortcut;
				if (ks.showEditDialog(e_hdlgMain)) {
					static_cast<Keystroke&>(*s_shortcut) = ks;
					updateItem();
					onItemUpdated((1 << kColKeystroke) | (1 << kColCond));
					updateList();
				}
			}
			break;
		
		
		case ID_TEXT_SENDKEYS:
			{
				Keystroke ks;
				if (ks.showSendKeysDialog(e_hdlgMain)) {
					TCHAR keystroke_display_name[kHotKeyBufSize];
					ks.getDisplayName(keystroke_display_name);
					
					String command = _T("[");
					escapeString(keystroke_display_name, &command);
					command += _T(']');
					
					appendText(command);
				}
			}
			break;
		
		case IDCCMD_HELP:
			{
				TCHAR language_code[MAX_PATH];
				i18n::loadStringAuto(IDS_LANGUAGE_CODE, language_code);
				
				TCHAR url[MAX_PATH];
				wsprintf(url, kHelpUrlFormat, language_code);
				
				ShellExecute(
					/* hwnd= */ NULL,
					/* lpOperation= */ nullptr,
					/* lpFile= */ url, /* lpParameters= */ nullptr,
					/* lpDirectory= */ nullptr,
					SW_SHOWDEFAULT);
			}
			break;
		
		
		case IDCCMD_ABOUT:
			i18n::dialogBox(IDD_ABOUT, e_hdlgMain, prcAbout);
			break;
		
		
		case IDCCMD_COPYLIST:
			{
				String shortcuts_csv;
				LVITEM lvi;
				lvi.mask = LVIF_PARAM;
				for (lvi.iItem = 0;; lvi.iItem++) {
					if (!ListView_GetItem(s_hwnd_list, &lvi)) {
						break;
					}
					reinterpret_cast<const Shortcut*>(lvi.lParam)->appendCsvLineToString(shortcuts_csv);
				}
				setClipboardText(shortcuts_csv);
				
				messageBox(e_hdlgMain, MSG_COPYLIST, MB_ICONINFORMATION);
			}
			break;
		
		
		// - Type text
		// - Run command line
		case IDCOPT_TEXT:
		case IDCOPT_COMMAND:
			{
				s_shortcut->m_type = (id == IDCOPT_COMMAND) ? Shortcut::Type::kCommand : Shortcut::Type::kText;
				s_shortcut->clearIcons();
				updateList();
				updateItem();
				SetFocus(GetDlgItem(e_hdlgMain, (id == IDCOPT_COMMAND) ? IDCTXT_COMMAND : IDCTXT_TEXT));
				onItemUpdated(1 << kColContents);
			}
			break;
		
		
		// Text, command, programs, description
		case IDCTXT_TEXT:
		case IDCTXT_COMMAND:
		case IDCTXT_PROGRAMS:
		case IDCTXT_DESCRIPTION:
			if (s_process_gui_events && notify == EN_CHANGE) {
				int col_index;
				String* col_contents;
				switch (id) {
					case IDCTXT_TEXT:
						col_contents = &s_shortcut->m_text;
						col_index = -1;
						break;
					case IDCTXT_COMMAND:
						col_contents = &s_shortcut->m_command;
						col_index = kColContents;
						break;
					case IDCTXT_PROGRAMS:
						col_contents = &s_shortcut->m_programs;
						col_index = kColCond;
						break;
					case IDCTXT_DESCRIPTION:
					default:
						col_contents = &s_shortcut->m_description;
						col_index = kColDescription;
						break;
				}
				
				getDlgItemText(e_hdlgMain, id, col_contents);
				
				if (id == IDCTXT_COMMAND) {
					s_shortcut->clearIcons();
					s_shortcut->getIcon();
				}
				
				updateItem();
				
				if (col_index >= 0) {
					onItemUpdated(1 << col_index);
				}
			}
			break;
		
		case IDCCBO_PROGRAMS:
			if (notify == CBN_SELCHANGE) {
				s_shortcut->m_programs_only = toBool(SendDlgItemMessage(e_hdlgMain,
					IDCCBO_PROGRAMS, CB_GETCURSEL, 0, 0));
				updateItem();
			}
			break;
		
		// Browse for command line
		case IDCCMD_COMMAND:
			if (browseForCommandLine(e_hdlgMain)) {
				SendMessage(e_hdlgMain, WM_COMMAND, MAKEWPARAM(IDCTXT_COMMAND, EN_KILLFOCUS),
					reinterpret_cast<LPARAM>(GetDlgItem(e_hdlgMain, IDCTXT_COMMAND)));
				updateItem();
			}
			break;
		
		case IDCCMD_CMDSETTINGS:
			if (IDOK == i18n::dialogBox(IDD_CMDSETTINGS, e_hdlgMain, prcCmdSettings)) {
				updateList();
				updateItem();
				onItemUpdated(1 << kColContents);
			}
			break;
		
		case IDCCMD_TEST:
			s_shortcut->execute(/* from_hotkey= */ false);
			break;
		
		case IDCCMD_TEXT_MENU:
			{
				const HMENU all_menus = i18n::loadMenu(IDM_CONTEXT);
				const HMENU menu = GetSubMenu(all_menus, 1);
				
				fillSpecialCharsMenu(menu, 5, ID_TEXT_SPECIALCHAR_FIRST);
				
				RECT dialog_rect;
				GetWindowRect(hwnd, &dialog_rect);
				SetFocus(GetDlgItem(e_hdlgMain, IDCTXT_TEXT));
				TrackPopupMenu(
					menu,
					TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON,
					dialog_rect.left, dialog_rect.bottom,
					/* nReserved= */ 0,
					e_hdlgMain,
					/* prcRect= */ nullptr);
				
				DestroyMenu(all_menus);
			}
			break;
		
		case ID_TEXT_COMMAND:
			{
				TCHAR file[MAX_PATH] = _T("");
				if (browseForCommandLine(e_hdlgMain, file, false)) {
					String command = _T("[[");
					escapeString(file, &command);
					command += _T("]]");
					appendText(command);
				}
			}
			break;
		
		case ID_UPDATE_LIST:
			updateList();
			break;
		
		default:
			if (ID_ADD_SPECIALCHAR_FIRST <= id && id < ID_ADD_ITEM_FIRST) {
				Shortcut *const shortcut = new Shortcut;
				const TCHAR text[] = { static_cast<TCHAR>(id - ID_ADD_SPECIALCHAR_FIRST), _T('\0') };
				shortcut->m_type = Shortcut::Type::kText;
				shortcut->m_text = text;
				addShortcut(shortcut);
			} else if (ID_TEXT_SPECIALCHAR_FIRST <= id && id < ID_TEXT_SPECIALCHAR_FIRST + 256) {
				const TCHAR text[] = { static_cast<TCHAR>(id - ID_TEXT_SPECIALCHAR_FIRST), _T('\0') };
				String command;
				escapeString(text, &command);
				appendText(command);
			} else if (ID_TEXT_LOWLEVEL <= id && id < ID_TEXT_LOWLEVEL + std::size(kWriteCommands)) {
				appendText(kWriteCommands[id - ID_TEXT_LOWLEVEL]);
			}
			break;
	}
}


INT_PTR CALLBACK prcMain(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam) {
	static RECT orig_client_rect;
	static SIZE min_size;
	
	switch (message) {
		
		// Initialization
		case WM_INITDIALOG:
			{
				e_hdlgMain = e_modal_dialog = hdlg;
				
				s_process_gui_events = false;
				
				subclassWindow(GetDlgItem(hdlg, IDCIMG_PROGRAMS), prcProgramsTarget);
				
				s_hwnd_list = GetDlgItem(hdlg, IDCLST);
				subclassWindow(s_hwnd_list, prcList);
				ListView_SetExtendedListViewStyle(s_hwnd_list, LVS_EX_FULLROWSELECT);
				
				// Get the initial size
				GetWindowRect(hdlg, &orig_client_rect);
				min_size.cx = orig_client_rect.right - orig_client_rect.left;
				min_size.cy = orig_client_rect.bottom - orig_client_rect.top;
				GetClientRect(hdlg, &orig_client_rect);
				
				// Get the initial position of controls
				for (int i = 0; i < arrayLength(main_dialog_placements); i++) {
					PLACEMENT &placement = main_dialog_placements[i];
					placement.hctl = GetDlgItem(hdlg, placement.id);
					RECT ctl_rect;
					GetWindowRect(placement.hctl, &ctl_rect);
					placement.size.cx = ctl_rect.right - ctl_rect.left;
					placement.size.cy = ctl_rect.bottom - ctl_rect.top;
					ScreenToClient(hdlg, reinterpret_cast<POINT*>(&ctl_rect));
					placement.point = reinterpret_cast<POINT&>(ctl_rect);
				}
				
				i18n::loadStringAuto(IDS_DONATEURL, s_donate_url);
				initializeWebLink(hdlg, IDCLBL_DONATE, s_donate_url);
				SendDlgItemMessage(hdlg, IDCLBL_DONATE, STM_SETIMAGE, IMAGE_BITMAP,
					reinterpret_cast<LPARAM>(i18n::loadBitmap(IDB_DONATE)));
				
				SendDlgItemMessage(hdlg, IDCCMD_TEXT_MENU, BM_SETIMAGE, IMAGE_ICON,
					reinterpret_cast<LPARAM>(i18n::loadNeutralIcon(IDI_COMBO, 11,6)));
				
				// Get the handle of the system image list for small icons
				SHFILEINFO shfi;
				s_sys_imagelist = (HIMAGELIST)SHGetFileInfo(
					_T("a"), FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(shfi),
					SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_SMALLICON | SHGFI_SYSICONINDEX);
				ListView_SetImageList(s_hwnd_list, s_sys_imagelist, LVSIL_SMALL);
				
				TCHAR text[512];
				
				// Add columns to the list
				i18n::loadStringAuto(IDS_COLUMNS, text);
				TCHAR *next_column = text;
				LVCOLUMN lvc;
				lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
				for (lvc.iSubItem = 0; lvc.iSubItem < kColCount; lvc.iSubItem++) {
					lvc.fmt = (lvc.iSubItem == kColUsageCount)
						? LVCFMT_RIGHT
						: LVCFMT_LEFT;
					lvc.pszText = const_cast<LPTSTR>(getSemiColonToken(&next_column));
					ListView_InsertColumn(s_hwnd_list, lvc.iSubItem, &lvc);
				}
				
				// Fill shortcuts list and unregister hot keys
				int shortcut_count = 0, shortcut_icon_count = 0;
				for (Shortcut *sh = shortcut::getFirst(); sh; sh = sh->getNext()) {
					Shortcut *const sh_copy = new Shortcut(*sh);
					sh_copy->unregisterHotKey();
					addItem(sh_copy, (s_shortcut == sh));
					shortcut_count++;
					if (sh_copy->m_type == Shortcut::Type::kCommand) {
						shortcut_icon_count++;
					}
				}
				onItemUpdated(~0);
				onShortcutsCountChanged();
				
				// Load small icons
				GETFILEICON **const apgfi = new GETFILEICON*[shortcut_icon_count + 1];
				apgfi[shortcut_icon_count] = nullptr;
				LVITEM lvi;
				lvi.mask = LVIF_PARAM;
				for (lvi.iItem = 0; lvi.iItem < shortcut_count ; lvi.iItem++) {
					ListView_GetItem(s_hwnd_list, &lvi);
					Shortcut *const psh = reinterpret_cast<Shortcut*>(lvi.lParam);
					if (psh->m_type == Shortcut::Type::kCommand) {
						psh->fillGetFileIcon(apgfi[--shortcut_icon_count] = new GETFILEICON, /* small_icon= */ true);
					}
				}
				startThread(reinterpret_cast<LPTHREAD_START_ROUTINE>(threadGetFilesIcon), apgfi);
				
				// Programs combo box
				const HWND programs_dropdown = GetDlgItem(hdlg, IDCCBO_PROGRAMS);
				TCHAR programs[kStringBufSize];
				i18n::loadStringAuto(IDS_PROGRAMS, programs);
				TCHAR *next_program = programs;
				for (int i = 0; i < 2; i++) {
					SendMessage(programs_dropdown, CB_ADDSTRING, 0,
						reinterpret_cast<LPARAM>(getSemiColonToken(&next_program)));
				}
				
				updateList();
				
				// Button icons and tooltips
				const HWND tooltip_hwnd = CreateWindowEx(
					WS_EX_TOPMOST,
					TOOLTIPS_CLASS, /* lpWindowName= */ NULL,
					WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
					CW_USEDEFAULT, CW_USEDEFAULT,
					CW_USEDEFAULT, CW_USEDEFAULT,
					hdlg, /* hMenu= */ NULL, e_instance, /* lpParam= */ nullptr);
				SetWindowPos(
					tooltip_hwnd,
					HWND_TOPMOST, 0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				
				i18n::loadStringAuto(IDS_TOOLTIPS, text);
				TCHAR *next_tooltip = text;
				TOOLINFO ti;
				ti.cbSize = sizeof(ti);
				ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
				ti.hinst = e_instance;
				for (int i = 0; i < 4; i++) {
					const UINT id = (i < 3) ? IDCCMD_ADD + i : IDCIMG_PROGRAMS;
					
					const HWND hctl = GetDlgItem(hdlg, id);
					if (i < 3) {
						SendMessage(hctl, BM_SETIMAGE, IMAGE_ICON,
							reinterpret_cast<LPARAM>(i18n::loadNeutralIcon(IDI_ADD + i, 15,15)));
					}
					
					ti.hwnd = hctl;
					ti.uId = reinterpret_cast<UINT_PTR>(hctl);
					ti.lpszText = const_cast<LPTSTR>(getSemiColonToken(&next_tooltip));
					SendMessage(tooltip_hwnd, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
				}
				
				// Auto start
				CheckDlgButton(hdlg, IDCCHK_AUTOSTART, isAutoStartEnabled());
				
				s_process_gui_events = true;
				
				SendMessage(hdlg, WM_SETICON, ICON_SMALL,
					reinterpret_cast<LPARAM>(i18n::loadNeutralIcon(IDI_APP, 16,16)));
				SendMessage(hdlg, WM_SETICON, ICON_BIG,
					reinterpret_cast<LPARAM>(i18n::loadNeutralIcon(IDI_APP, 32,32)));
				
				// Restore window size from settings
				if (e_main_dialog_size.cx < min_size.cx || e_main_dialog_size.cy < min_size.cy) {
					e_main_dialog_size = min_size;
				} else{
					// Default settings: center window in the default monitor.
					
					const HMONITOR monitor = MonitorFromWindow(hdlg, MONITOR_DEFAULTTOPRIMARY);
					MONITORINFO mi;
					mi.cbSize = sizeof(mi);
					GetMonitorInfo(monitor, &mi);
					
					WINDOWPLACEMENT dialog_wp;
					GetWindowPlacement(hdlg, &dialog_wp);
					dialog_wp.rcNormalPosition.left = (mi.rcWork.left + mi.rcWork.right - e_main_dialog_size.cx) / 2;
					dialog_wp.rcNormalPosition.top = (mi.rcWork.top + mi.rcWork.bottom - e_main_dialog_size.cy) / 2;
					dialog_wp.rcNormalPosition.right = dialog_wp.rcNormalPosition.left + e_main_dialog_size.cx;
					dialog_wp.rcNormalPosition.bottom = dialog_wp.rcNormalPosition.top + e_main_dialog_size.cy;
					dialog_wp.showCmd = e_maximize_main_dialog ? SW_MAXIMIZE : SW_RESTORE;
					SetWindowPlacement(hdlg, &dialog_wp);
				}
				
				centerParent(hdlg);
				
				SendMessage(hdlg, WM_SIZE, 0, 0);
				
				if (lParam) {
					PostMessage(hdlg, WM_COMMAND, static_cast<WPARAM>(lParam), 0);
				}
				
				DragAcceptFiles(hdlg, true);
			}
			return true;
		
		
		// Command
		case WM_COMMAND:
			onMainCommand(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
			break;
		
		
		// Draw the bottom-right gripper
		case WM_PAINT:
			{
				RECT dialog_rect;
				GetClientRect(hdlg, &dialog_rect);
				dialog_rect.left = dialog_rect.right - GetSystemMetrics(SM_CXHSCROLL);
				dialog_rect.top = dialog_rect.bottom - GetSystemMetrics(SM_CYVSCROLL);
				
				RECT update_rect;
				if (GetUpdateRect(hdlg, &update_rect, /* bErase= */ true) &&
						IntersectRect(&update_rect, &dialog_rect, &update_rect)) {
					PAINTSTRUCT ps;
					const HDC hdc = BeginPaint(hdlg, &ps);
					DrawFrameControl(hdc, &dialog_rect, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
					EndPaint(hdlg, &ps);
				}
			}
			break;
		
		// Behavior of the bottom-right gripper:
		// same as the bottom-right corner of the dialog box
		case WM_NCHITTEST:
			{
				RECT dialog_rect;
				GetClientRect(hdlg, &dialog_rect);
				ClientToScreen(hdlg, reinterpret_cast<POINT*>(&dialog_rect) + 1);
				dialog_rect.left = dialog_rect.right - GetSystemMetrics(SM_CXHSCROLL);
				dialog_rect.top = dialog_rect.bottom - GetSystemMetrics(SM_CYVSCROLL);
				
				const POINT hit_point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				if (PtInRect(&dialog_rect, hit_point)) {
					SetWindowLongPtr(hdlg, DWLP_MSGRESULT, HTBOTTOMRIGHT);
					return true;
				}
			}
			break;
		
		
		// Notification (from list)
		case WM_NOTIFY:
			if (wParam == IDCLST) {
				NMHDR *const nmhdr = reinterpret_cast<NMHDR*>(lParam);
				switch (nmhdr->code) {
					
					case LVN_ITEMACTIVATE:
						PostMessage(hdlg, WM_COMMAND, IDCCMD_EDIT, 0);
						break;
					
					case LVN_GETDISPINFO:
						{
							LVITEM &lvi = reinterpret_cast<NMLVDISPINFO*>(lParam)->item;
							Shortcut *const sh = reinterpret_cast<Shortcut*>(lvi.lParam);
							if (lvi.mask & LVIF_IMAGE) {
								lvi.iImage = sh->getSmallIconIndex();
							}
							if (lvi.mask & LVIF_TEXT) {
								String column_text;
								sh->getColumnText(lvi.iSubItem, column_text);
								lstrcpyn(lvi.pszText, column_text, lvi.cchTextMax);
							}
						}
						break;
					
					case LVN_ITEMCHANGED:
						// Delay updateList() after all LVN_ITEMCHANGED to avoid flickering
						// (one LVN_ITEMCHANGED to unselect the old item, another to select the new item).
						// Do nothing if s_process_gui_events is true (during addCreatedShortcut)
						// to avoid resetting the edit box selection.
						if (s_process_gui_events) {
							const NMLISTVIEW& lv = *reinterpret_cast<NMLISTVIEW*>(lParam);
							if ((lv.uOldState ^ lv.uNewState) & LVIS_SELECTED) {
								PostMessage(hdlg, WM_COMMAND, ID_UPDATE_LIST, 0);
							}
						}
						break;
					
					case LVN_COLUMNCLICK:
						Shortcut::s_sort_column = reinterpret_cast<const NMLISTVIEW*>(lParam)->iSubItem;
						onItemUpdated(~0);
						break;
				}
				return 0;
				
			} else if (wParam == 0 && reinterpret_cast<NMHDR*>(lParam)->code == HDN_ENDTRACK) {
				// Get columns size
				LVCOLUMN lvc;
				lvc.mask = LVCF_WIDTH;
				RECT list_rect;
				GetClientRect(s_hwnd_list, &list_rect);
				for (lvc.iSubItem = 0; lvc.iSubItem < kSizedColumnCount; lvc.iSubItem++) {
					ListView_GetColumn(s_hwnd_list, lvc.iSubItem, &lvc);
					e_column_widths[lvc.iSubItem] = MulDiv(lvc.cx, 100, list_rect.right);
				}
			}
			break;
		
		
		case WM_GETMINMAXINFO:
			reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize =
				reinterpret_cast<POINT&>(min_size);
			break;
		
		case WM_SIZE:
			{
				RECT new_client_rect;
				GetClientRect(hdlg, &new_client_rect);
				int dx = new_client_rect.right - orig_client_rect.right;
				int dy = new_client_rect.bottom - orig_client_rect.bottom;
				
				for (int i = 0; i < arrayLength(main_dialog_placements); i++) {
					main_dialog_placements[i].updateGui(dx, dy);
				}
				
				// Resize columns
				LVCOLUMN lvc;
				lvc.mask = LVCF_WIDTH;
				RECT list_rect;
				GetClientRect(s_hwnd_list, &list_rect);
				int remaining_width = list_rect.right;
				for (lvc.iSubItem = 0; lvc.iSubItem < kColCount; lvc.iSubItem++) {
					lvc.cx = (lvc.iSubItem < kSizedColumnCount)
						? MulDiv(e_column_widths[lvc.iSubItem], list_rect.right, 100)
						: remaining_width;
					remaining_width -= lvc.cx;
					ListView_SetColumn(s_hwnd_list, lvc.iSubItem, &lvc);
				}
				
				RedrawWindow(
					hdlg, /* lprcUpdate= */ nullptr, /* hrgnUpdate= */ NULL,
					RDW_INVALIDATE | RDW_ERASE | RDW_NOFRAME);
				
				WINDOWPLACEMENT dialog_wp;
				GetWindowPlacement(hdlg, &dialog_wp);
				e_main_dialog_size.cx = dialog_wp.rcNormalPosition.right - dialog_wp.rcNormalPosition.left;
				e_main_dialog_size.cy = dialog_wp.rcNormalPosition.bottom - dialog_wp.rcNormalPosition.top;
				e_maximize_main_dialog = (dialog_wp.showCmd == SW_MAXIMIZE);
			}
			break;
		
		
		// Owner draw menu item: populate
		case WM_INITMENUPOPUP:
			if (!HIWORD(lParam)) {
				const HMENU menu = reinterpret_cast<HMENU>(wParam);
				MENUITEMINFO mii;
				mii.cbSize = sizeof(mii);
				mii.fMask = MIIM_DATA | MIIM_TYPE;
				mii.cch = 0;
				if (GetMenuItemCount(menu) == 1 &&
						GetMenuItemInfo(menu, 0, /* fByPosition= */ true, &mii) &&
						(mii.fType & MFT_SEPARATOR)) {
					RemoveMenu(menu, 0, MF_BYPOSITION);
					BaseMenuItem::forItemData(mii.dwItemData)->populate(menu);
				}
			}
			break;
		
		// Owner draw menu item: compute size
		case WM_MEASUREITEM:
			{
				MEASUREITEMSTRUCT* mis = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
				if (mis->CtlType == ODT_MENU && mis->itemData) {
					BaseMenuItem::forItemData(mis->itemData)->measureItem(mis);
					return true;
				}
			}
			break;
		
		// Owner draw menu item: draw
		case WM_DRAWITEM:
			{
				const DRAWITEMSTRUCT &dis = *reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
				if (dis.CtlType == ODT_MENU && dis.itemData) {
					BaseMenuItem::forItemData(dis.itemData)->drawItem(dis);
					return true;
				}
			}
			break;
		
		// End of threadGetFileIcon
		case WM_GETFILEICON:
			if (lParam) {
				GETFILEICON *const pgfi = reinterpret_cast<GETFILEICON*>(lParam);
				
				LVITEM lvi;
				lvi.mask = LVIF_PARAM;
				for (lvi.iItem = ListView_GetItemCount(s_hwnd_list); --lvi.iItem >= 0;) {
					ListView_GetItem(s_hwnd_list, &lvi);
					Shortcut *const psh = reinterpret_cast<Shortcut*>(lvi.lParam);
					if (psh == pgfi->shortcut) {
						psh->onGetFileInfo(*pgfi);
					}
				}
				
				delete pgfi;
			}
			break;
		
		
		case WM_DROPFILES:
			{
				TCHAR path[MAX_PATH];
				if (DragQueryFile(reinterpret_cast<HDROP>(wParam), /* iFile= */ 0, path, arrayLength(path))) {
					addPathShortcut(path);
				}
			}
			break;
	}
	
	return 0;
}


// Associates VK_DELETE with IDCCMD_DELETE.
LRESULT CALLBACK prcList(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR UNUSED(subclass_id), DWORD_PTR UNUSED(ref_data)) {
	switch (message) {
		case WM_KEYDOWN:
			if (wParam == VK_DELETE) {
				PostMessage(e_hdlgMain, WM_COMMAND, IDCCMD_DELETE, 0);
			}
			break;
	}
	
	return DefSubclassProc(hwnd, message, wParam, lParam);
}


LRESULT CALLBACK prcProgramsTarget(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR UNUSED(subclass_id), DWORD_PTR UNUSED(ref_data)) {
	switch (message) {
		case WM_LBUTTONDOWN:
			if (s_shortcut) {
				s_capturing_program = true;
				SetCapture(hwnd);
				SetCursor(LoadCursor(e_instance, MAKEINTRESOURCE(IDC_TARGET)));
				s_old_programs = s_shortcut->m_programs;
			}
			break;
		
		case WM_MOUSEMOVE:
			if (s_capturing_program && s_shortcut) {
				POINT mouse_pos;
				GetCursorPos(&mouse_pos);
				const HWND targeted_window = WindowFromPoint(mouse_pos);
				
				String new_programs = s_old_programs;
				DWORD idProcess;
				TCHAR path[MAX_PATH];
				if (GetWindowThreadProcessId(targeted_window, &idProcess) &&
						idProcess != GetCurrentProcessId() &&
						getWindowProcessName(targeted_window, path)) {
					if (new_programs.isSome()) {
						new_programs += _T(';');
					}
					new_programs += path;
				}
				
				String old_programs;
				getDlgItemText(e_hdlgMain, IDCTXT_PROGRAMS, &old_programs);
				if (lstrcmp(old_programs, new_programs)) {
					SetDlgItemText(e_hdlgMain, IDCTXT_PROGRAMS, new_programs);
				}
			}
			break;
		
		case WM_LBUTTONUP:
			if (s_capturing_program) {
				s_capturing_program = false;
				ReleaseCapture();
				if (s_shortcut) {
					if (lstrcmp(s_shortcut->m_programs, s_old_programs)) {
						s_shortcut->cleanPrograms();
						SetDlgItemText(e_hdlgMain, IDCTXT_PROGRAMS, s_shortcut->m_programs);
					} else {
						messageBox(hwnd, ERR_TARGET, MB_ICONEXCLAMATION);
					}
				}
			}
			break;
			
		case WM_CAPTURECHANGED:
			if (s_capturing_program) {
				s_capturing_program = false;
				if (s_shortcut) {
					SetDlgItemText(e_hdlgMain, IDCTXT_PROGRAMS, s_old_programs);
				}
			}
			break;
	}
	
	return DefSubclassProc(hwnd, message, wParam, lParam);
}



INT_PTR CALLBACK prcCmdSettings(HWND hdlg, UINT message, WPARAM wParam, LPARAM UNUSED(lParam)) {
	switch (message) {
		case WM_INITDIALOG:
			{
				e_modal_dialog = hdlg;
				centerParent(hdlg);
				
				// Fill "command line show" list
				String show_options(IDS_SHOW_OPTIONS);
				TCHAR* next_show_option = show_options.get();
				for (int i = 0; i < arrayLength(Shortcut::kShowOptions); i++) {
					LPCTSTR show_option = show_options.isEmpty()
						? getToken(Token::kShowNormal + i)
						: getSemiColonToken(&next_show_option);
					SendDlgItemMessage(hdlg, IDCCBO_SHOW, CB_ADDSTRING, 0,
							reinterpret_cast<LPARAM>(show_option));
				}
				
				SetDlgItemText(hdlg, IDCTXT_COMMAND, s_shortcut->m_command);
				SetDlgItemText(hdlg, IDCTXT_DIRECTORY, s_shortcut->m_directory);
				
				int show_option_index;
				for (show_option_index = 0; show_option_index < arrayLength(Shortcut::kShowOptions); show_option_index++) {
					if (Shortcut::kShowOptions[show_option_index] == s_shortcut->m_show_option) {
						break;
					}
				}
				SendDlgItemMessage(hdlg, IDCCBO_SHOW, CB_SETCURSEL, static_cast<WPARAM>(show_option_index), 0);
				
				CheckDlgButton(hdlg, IDCCHK_SUPPORTFILEOPEN, s_shortcut->m_support_file_open);
			}
			return true;
		
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				
				// Browse for command line
				case IDCCMD_COMMAND:
					browseForCommandLine(hdlg);
					break;
				
				// Browse for directory
				case IDCCMD_DIRECTORY:
					{
						// Get the command line file (without arguments)
						TCHAR directory[MAX_PATH];
						GetDlgItemText(hdlg, IDCTXT_DIRECTORY, directory, arrayLength(directory));
						
						TCHAR dialog_title[MAX_PATH];
						i18n::loadStringAuto(IDS_BROWSEDIR, dialog_title);
						if (!browseForFolder(hdlg, dialog_title, directory)) {
							break;
						}
						
						// Update internal structures
						SetDlgItemText(hdlg, IDCTXT_DIRECTORY, directory);
						SendMessage(hdlg, WM_COMMAND, MAKEWPARAM(IDCTXT_DIRECTORY, EN_KILLFOCUS),
								reinterpret_cast<LPARAM>(GetDlgItem(hdlg, IDCTXT_DIRECTORY)));
					}
					break;
				
				case IDOK:
					getDlgItemText(hdlg, IDCTXT_COMMAND, &s_shortcut->m_command);
					getDlgItemText(hdlg, IDCTXT_DIRECTORY, &s_shortcut->m_directory);
					s_shortcut->m_show_option = Shortcut::kShowOptions[
						SendDlgItemMessage(hdlg, IDCCBO_SHOW, CB_GETCURSEL, 0,0)];
					s_shortcut->m_support_file_open = toBool(IsDlgButtonChecked(hdlg, IDCCHK_SUPPORTFILEOPEN));
					s_shortcut->clearIcons();
					// Fall-through
					
				case IDCANCEL:
					e_modal_dialog = e_hdlgMain;
					EndDialog(hdlg, LOWORD(wParam));
					break;
			}
			break;
	}
	
	return 0;
}


INT_PTR CALLBACK prcLanguage(HWND hdlg, UINT message, WPARAM wParam, LPARAM UNUSED(lParam)) {
	const HWND list_hwnd = GetDlgItem(hdlg, IDCLST_LANGUAGE);
	
	switch (message) {
		case WM_INITDIALOG:
			{
				e_modal_dialog = hdlg;
				
				for (int lang = 0; lang < i18n::kLangCount; lang++) {
					ListBox_AddString(list_hwnd, getLanguageName(static_cast<i18n::Language>(lang)));
				}
				ListBox_SetCurSel(list_hwnd, i18n::getLanguage());
			}
			return true;
		
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDCLST_LANGUAGE:
					if (HIWORD(wParam) != LBN_DBLCLK) {
						break;
					}
				// Fall-through
				case IDOK:
					{
						const i18n::Language lang =
							static_cast<i18n::Language>(ListBox_GetCurSel(list_hwnd));
						if (i18n::getLanguage() != lang) {
							i18n::setLanguage(lang);
							// No need to save the configuration: onMainCommand() will do it.
						} else {
							wParam = IDCANCEL;
						}
					}
				case IDCANCEL:
					EndDialog(hdlg, LOWORD(wParam));
					break;
			}
			break;
	}
	
	return 0;
}



INT_PTR CALLBACK prcAbout(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_INITDIALOG:
		{
			e_modal_dialog = hdlg;
			centerParent(hdlg);
			
			// Extract the version and copyright notice from resources,
			// then populate the full app version: "Clavier+ <version>\n<copyright>".
			TCHAR exe_filepath[MAX_PATH];
			GetModuleFileName(e_instance, exe_filepath, arrayLength(exe_filepath));
			DWORD version_size = GetFileVersionInfoSize(exe_filepath, /* lpdwHandle= */ nullptr);
			BYTE *const version_buffer = new BYTE[version_size];
			if (GetFileVersionInfo(exe_filepath, /* dwHandle= */ 0, version_size, version_buffer)) {
				LPTSTR product_version, legal_copyright;
				UINT production_version_len, legal_copyright_len;
				if (VerQueryValue(version_buffer, _T("\\StringFileInfo\\000004b0\\ProductVersion"),
						reinterpret_cast<LPVOID*>(&product_version), &production_version_len)
					&& VerQueryValue(version_buffer, _T("\\StringFileInfo\\000004b0\\LegalCopyright"),
						reinterpret_cast<LPVOID*>(&legal_copyright), &legal_copyright_len)) {
					TCHAR app_version[100];
					wsprintf(app_version, _T("%s %s\n%s"), kAppName, product_version, legal_copyright);
					SetDlgItemText(hdlg, IDCLBL_APP_VERSION, app_version);
				}
			}
			delete [] version_buffer;
			
			initializeWebLink(hdlg, IDCLBL_WEBSITE, _T("https://gryder.org/software/clavier-plus/"));
			initializeWebLink(hdlg, IDCLBL_EMAIL, _T("mailto:guillaume@ryder.fr"));
			i18n::loadStringAuto(IDS_DONATEURL, s_donate_url);
			initializeWebLink(hdlg, IDCLBL_DONATE, s_donate_url);
			SendDlgItemMessage(hdlg, IDCLBL_DONATE, STM_SETIMAGE, IMAGE_BITMAP,
				reinterpret_cast<LPARAM>(i18n::loadBitmap(IDB_DONATE)));
			return true;
		}
		
		case WM_CTLCOLORSTATIC:
			{
				const HDC hdc = reinterpret_cast<HDC>(wParam);
				const HWND control_hwnd = reinterpret_cast<HWND>(lParam);
				const int id = GetDlgCtrlID(control_hwnd);
				if (id == IDCLBL_EMAIL || id == IDCLBL_WEBSITE) {
					HFONT font = reinterpret_cast<HFONT>(GetWindowLongPtr(hdlg, DWLP_USER));
					if (!font) {
						LOGFONT lf;
						GetObject(reinterpret_cast<HFONT>(SendMessage(control_hwnd, WM_GETFONT, 0,0)), sizeof(lf), &lf);
						lf.lfUnderline = true;
						font = CreateFontIndirect(&lf);
						SetWindowLongPtr(hdlg, DWLP_USER, reinterpret_cast<LONG_PTR>(font));
					}
					SelectFont(hdc, font);
					SetTextColor(hdc, RGB(0,0,255));
					SetBkMode(hdc, TRANSPARENT);
					return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_BTNFACE));
				}
			}
			break;
		
		case WM_COMMAND:
			DeleteFont(reinterpret_cast<HFONT>(GetWindowLongPtr(hdlg, DWLP_USER)));
			EndDialog(hdlg, IDOK);
			break;
	}
	
	return 0;
}


void updateItem() {
	const int item_index = ListView_GetNextItem(s_hwnd_list, -1, LVNI_SELECTED);
	ListView_RedrawItems(s_hwnd_list, /* iFirst= */ item_index, /* iLast= */ item_index);
}


void updateList() {
	s_process_gui_events = false;
	
	Shortcut *const shortcut = getSelectedShortcut();
	const bool selected = toBool(shortcut);
	
	HICON icon = NULL;
	
	if (selected) {
		// Fill shortcut controls
		
		s_shortcut = shortcut;
		CheckRadioButton(e_hdlgMain, IDCOPT_TEXT, IDCOPT_COMMAND,
			(s_shortcut->m_type == Shortcut::Type::kCommand) ? IDCOPT_COMMAND : IDCOPT_TEXT);
		
		SetDlgItemText(e_hdlgMain, IDCTXT_TEXT, s_shortcut->m_text);
		SetDlgItemText(e_hdlgMain, IDCTXT_COMMAND, s_shortcut->m_command);
		SetDlgItemText(e_hdlgMain, IDCTXT_PROGRAMS, s_shortcut->m_programs);
		SetDlgItemText(e_hdlgMain, IDCTXT_DESCRIPTION, s_shortcut->m_description);
		SendDlgItemMessage(e_hdlgMain, IDCCBO_PROGRAMS, CB_SETCURSEL,
			s_shortcut->m_programs_only, 0);
		
		icon = s_shortcut->getIcon();
	}
	
	// Enable or disable shortcut controls and Delete button
	const bool is_command = toBool(IsDlgButtonChecked(e_hdlgMain, IDCOPT_COMMAND));
	constexpr UINT kControlIds[] = {
		IDCCMD_DELETE, IDCCMD_EDIT, IDCFRA_TEXT, IDCFRA_COMMAND,
		IDCOPT_TEXT, IDCOPT_COMMAND,
		IDCLBL_DESCRIPTION, IDCTXT_DESCRIPTION,
		IDCLBL_PROGRAMS, IDCCBO_PROGRAMS, IDCTXT_PROGRAMS, IDCIMG_PROGRAMS,
		IDCTXT_TEXT, IDCCMD_TEXT_MENU,
		IDCTXT_COMMAND, IDCCMD_COMMAND, IDCCMD_CMDSETTINGS, IDCCMD_TEST,
	};
	const bool controls_enabled[] = {
		true, true, true, true,
		true, true,
		true, true,
		true, true, true, true,
		!is_command, !is_command,
		is_command, is_command, is_command, is_command,
	};
	for (int i = 0; i < arrayLength(kControlIds); i++) {
		EnableWindow(GetDlgItem(e_hdlgMain, kControlIds[i]), selected && controls_enabled[i]);
	}
	
	// Icon
	SendDlgItemMessage(
		e_hdlgMain, IDCLBL_ICON, STM_SETICON,
		reinterpret_cast<WPARAM>(is_command ? icon : NULL), 0);
	
	s_process_gui_events = true;
}


int addItem(Shortcut* shortcut, bool selected) {
	LVITEM lvi;
	lvi.iItem = 0;
	lvi.iSubItem = 0;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	lvi.state = selected ? LVIS_SELECTED | LVIS_FOCUSED : 0;
	lvi.lParam = reinterpret_cast<LPARAM>(shortcut);
	lvi.iImage = I_IMAGECALLBACK;
	lvi.iItem = ListView_InsertItem(s_hwnd_list, &lvi);
	lvi.mask = LVIF_TEXT;
	for (lvi.iSubItem = 1; lvi.iSubItem < arrayLength(e_column_widths); lvi.iSubItem++) {
		ListView_SetItem(s_hwnd_list, &lvi);
	}
	
	return lvi.iItem;
}


void onItemUpdated(int columns_mask) {
	if (columns_mask & (1 << Shortcut::s_sort_column)) {
		ListView_SortItems(s_hwnd_list, Shortcut::compare, /* lParam= */ 0);
	}
	ListView_EnsureVisible(s_hwnd_list, ListView_GetSelectionMark(s_hwnd_list), false);
}


void onShortcutsCountChanged() {
	SetDlgItemInt(
		e_modal_dialog, IDCTXT_SHORTCUTSCOUNT,
		static_cast<UINT>(ListView_GetItemCount(s_hwnd_list)), /* bSigned= */ true);
}


Shortcut* getSelectedShortcut() {
	LVITEM lvi;
	lvi.iItem = ListView_GetNextItem(s_hwnd_list, -1, LVNI_SELECTED);
	if (lvi.iItem < 0) {
		return nullptr;
	}
	lvi.mask = LVIF_PARAM;
	ListView_GetItem(s_hwnd_list, &lvi);
	return reinterpret_cast<Shortcut*>(lvi.lParam);
}

}  // namespace
}  // namespace dialogs


//------------------------------------------------------------------------
// Shortcut
//------------------------------------------------------------------------

namespace dialogs {
namespace {

// Get an icon, then give it to the main window by posting a WM_GETFILEICON message.
DWORD WINAPI threadGetFileIcon(dialogs::GETFILEICON& gfi) {
	// Use HTML files icon for URLs
	if (PathIsURL(gfi.executable)) {
		lstrcpy(gfi.executable, _T("a.url"));
		gfi.flags |= SHGFI_USEFILEATTRIBUTES;
	}
	
	gfi.ok = getFileInfo(gfi.executable, 0, gfi.shfi, gfi.flags);
	
	PostMessage(dialogs::e_hdlgMain, WM_GETFILEICON, 0, reinterpret_cast<LPARAM>(&gfi));
	return 0;
}

// Get several icons, then delete the GETFILEICON* array.
DWORD WINAPI threadGetFilesIcon(dialogs::GETFILEICON* apgfi[]) {
	for (int i = 0; apgfi[i]; i++) {
		threadGetFileIcon(*apgfi[i]);
	}
	delete [] apgfi;
	return 0;
}

}  // namespace
}  // namespace dialogs

namespace shortcut {

void Shortcut::findExecutable(LPTSTR executable) {
	TCHAR file[MAX_PATH];
	lstrcpyn(file, m_command, arrayLength(file));
	PathRemoveArgs(file);
	findFullPath(file, executable);
}


void Shortcut::onGetFileInfo(dialogs::GETFILEICON& gfi) {
	if (gfi.flags & SHGFI_SYSICONINDEX) {
		// Small icon index
		
		m_small_icon_index = gfi.ok ? gfi.shfi.iIcon : kIconInvalid;
		
		LVFINDINFO lvfi;
		lvfi.flags = LVFI_PARAM;
		lvfi.lParam = reinterpret_cast<LPARAM>(gfi.shortcut);
		const int iItem = ListView_FindItem(dialogs::s_hwnd_list, -1, &lvfi);
		if (iItem >= 0) {
			ListView_RedrawItems(dialogs::s_hwnd_list, iItem,iItem);
		}
		
	} else {
		// Big icon
		
		m_icon = gfi.ok ? gfi.shfi.hIcon : NULL;
		if (dialogs::s_shortcut == this) {
			SendDlgItemMessage(dialogs::e_hdlgMain, IDCLBL_ICON, STM_SETICON,
				reinterpret_cast<WPARAM>((m_type == Shortcut::Type::kCommand) ? m_icon : nullptr), 0);
		}
	}
}

void Shortcut::findSmallIconIndex() {
	VERIFV(m_small_icon_index != kIconThreadRunning);
	m_small_icon_index = kIconThreadRunning;
	
	if (m_type == Shortcut::Type::kCommand) {
		fillGetFileIcon(/* pgfi= */ nullptr, true);
	}
}

void Shortcut::fillGetFileIcon(dialogs::GETFILEICON* pgfi, bool small_icon) {
	const bool start = !pgfi;
	if (start) {
		pgfi = new dialogs::GETFILEICON;
	}
	
	if (small_icon) {
		m_small_icon_index = kIconThreadRunning;
	}
	
	pgfi->shortcut = this;
	pgfi->flags = small_icon
		? SHGFI_ICON | SHGFI_SMALLICON | SHGFI_SYSICONINDEX
		: SHGFI_ICON;
	findExecutable(pgfi->executable);
	
	if (start) {
		if (*pgfi->executable) {
			startThread(reinterpret_cast<LPTHREAD_START_ROUTINE>(dialogs::threadGetFileIcon), pgfi);
		} else {
			dialogs::threadGetFileIcon(*pgfi);
		}
	}
}

void Shortcut::findIcon() {
	fillGetFileIcon(/* pgfi= */ nullptr, /* small_icon= */ false);
}

int Shortcut::getSmallIconIndex() {
	if (m_type != Shortcut::Type::kCommand) {
		return -1;
	}
	if (m_small_icon_index == kIconNeeded) {
		findSmallIconIndex();
	}
	return m_small_icon_index;
}

void Shortcut::clearIcons() {
	m_small_icon_index = kIconNeeded;
	if (m_icon) {
		DestroyIcon(m_icon);
		m_icon = NULL;
	}
}


void Shortcut::appendCsvLineToString(String& output) const {
	TCHAR display_name[kHotKeyBufSize];
	getDisplayName(display_name);
	output += display_name;
	output += _T('\t');
	
	output += getToken((m_type == Shortcut::Type::kCommand) ? Token::kCommand : Token::kText);
	output += _T('\t');
	
	output += (m_type == Shortcut::Type::kCommand) ? m_command : m_text;
	output += _T('\t');
	
	String cond;
	getColumnText(kColCond, cond);
	output += cond;
	output += _T('\t');
	
	output += m_description;
	output += _T("\r\n");
}


void Shortcut::getColumnText(int column_text, String& output) const {
	switch (column_text) {
		
		case kColContents:
			output = (m_type == Shortcut::Type::kCommand) ? m_command : m_text;
			break;
		
		case kColKeystroke:
			getDisplayName(output.getBuffer(kHotKeyBufSize));
			break;
		
		case kColCond:
			{
				for (int cond = 0; cond < Keystroke::kConditionCount; cond++) {
					TCHAR cond_chr = Keystroke::kConditionChars[static_cast<int>(m_conditions[cond])];
					if (cond_chr) {
						output += cond_chr;
						output += dialogs::s_cond_key_translations.get()[cond];
					}
				}
				if (m_programs.isSome()) {
					if (output.isSome()) {
						output += _T(' ');
					}
					output += m_programs_only ? _T('+') : _T('-');
					output += m_programs;
				}
			}
			break;
		
		case kColUsageCount:
			i18n::formatInteger(m_usage_count, &output);
			break;
		
		case kColDescription:
			output = m_description;
			break;
	}
}


int Shortcut::s_sort_column = kColContents;

int CALLBACK Shortcut::compare(const Shortcut* shortcut1, const Shortcut* shortcut2, LPARAM UNUSED(lParam)) {
	
	// Some columns needing a special treatment
	switch (s_sort_column) {
		case kColContents:
			if (shortcut1->m_type != shortcut2->m_type) {
				// Type has precedence over contents.
				return static_cast<int>(shortcut1->m_type) - static_cast<int>(shortcut2->m_type);
			}
			break;
		
		case kColKeystroke:
			return Keystroke::compare(*shortcut1, *shortcut2);
		
		case kColUsageCount:
			return shortcut1->m_usage_count - shortcut2->m_usage_count;
	}
	
	// Other columns: sort alphabetically.
	String text1, text2;
	shortcut1->getColumnText(s_sort_column, text1);
	shortcut2->getColumnText(s_sort_column, text2);
	return lstrcmpi(text1, text2);
}

}  // namespace shortcut


namespace dialogs {
namespace {

void fillSpecialCharsMenu(HMENU menu, int pos, UINT first_id) {
	const HMENU sub_menu = GetSubMenu(menu, pos);
	RemoveMenu(sub_menu, 0, MF_BYPOSITION);
	for (int chr = 128; chr < 256; chr++) {
		TCHAR label[30];
		wsprintf(label, _T("%d - %c"), chr, chr);
		AppendMenu(sub_menu,
			((chr & 15) == 0) ? MF_STRING | MF_MENUBREAK : MF_STRING,
			first_id + chr, label);
	}
}

void escapeString(LPCTSTR input, String* output) {
	int output_len = output->getLength();
	TCHAR* output_end = output->getBuffer(output_len + lstrlen(input) + 1) + output_len;
	for (;;) {
		switch (*input) {
			case '\0':
				*output_end = _T('\0');
				return;
			case '\\':
			case '[':
			case ']':
			case '{':
			case '}':
			case '|':
				*output_end++ = _T('\\');
			default:
				*output_end++ = *input++;
				break;
		}
	}
}

void appendText(LPCTSTR text) {
	const HWND hctl = GetDlgItem(e_hdlgMain, IDCTXT_TEXT);
	Edit_ReplaceSel(hctl, text);
	SetFocus(hctl);
}

}  // namespace
}  // namespace dialogs
