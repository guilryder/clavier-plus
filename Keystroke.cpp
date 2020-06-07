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
#include "Keystroke.h"


const SpecialKey e_special_keys[kSpecialKeysCount] = {
	{ VK_LWIN, VK_LWIN, VK_RWIN, MOD_WIN, tokWin },
	{ VK_CONTROL, VK_LCONTROL, VK_RCONTROL, MOD_CONTROL, tokCtrl },
	{ VK_SHIFT, VK_LSHIFT, VK_RSHIFT, MOD_SHIFT, tokShift },
	{ VK_MENU, VK_LMENU, VK_RMENU, MOD_ALT, tokAlt },
};


// Pointer to the old value of the currently edited keystroke
static Keystroke *s_old_edited_keystroke;

// True if the currently edited keystroke should be reset ASAP.
// The keystroke is reset when new keys are pressed after all special keys have been released.
static bool s_reset_keystroke;

// Currently edited keystroke
static Keystroke s_keystroke;

// Default window proc of edit controls. Used by prcKeystrokeCtl.
static WNDPROC s_prcKeystrokeCtl;

// Window proc an edit box allowing to enter a keystroke.
static LRESULT CALLBACK prcKeystrokeCtl(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


// Shortcut keystroke and conditions dialog box
INT_PTR CALLBACK prcKeystroke(HWND hDlg, UINT message, WPARAM wParam, LPARAM) {
	switch (message) {
		
		// Initialization
		case WM_INITDIALOG:
			{
				e_hdlgModal = hDlg;
				centerParent(hDlg);
				
				// Subclass the keystroke control. Display the initial keystroke name.
				const HWND hwnd_keystroke = GetDlgItem(hDlg, IDCTXT);
				s_prcKeystrokeCtl = subclassWindow(hwnd_keystroke, prcKeystrokeCtl);
				PostMessage(hwnd_keystroke, WM_KEYSTROKE, 0,0);
				
				CheckDlgButton(hDlg, IDCCHK_DISTINGUISH_LEFT_RIGHT, s_keystroke.m_sided);
				
				// Load ';'-separated condition names. Make them null-terminated strings.
				TCHAR conditions[bufString];
				i18n::loadStringAuto(IDS_CONDITIONS, conditions);
				TCHAR* current_condition = conditions;
				for (int cond = 0; cond < condCount; cond++) {
					getSemiColonToken(current_condition);
				}
				
				// Initialize the condition combo-boxes
				for (int cond_type = 0; cond_type < condTypeCount; cond_type++) {
					const HWND hwnd_cond_type = GetDlgItem(hDlg, IDCCBO_CAPSLOCK + cond_type);
					current_condition = conditions;
					for (int cond = 0; cond < condCount; cond++) {
						ComboBox_AddString(hwnd_cond_type, current_condition);
						current_condition += lstrlen(current_condition) + 1;
					}
					ComboBox_SetCurSel(hwnd_cond_type, s_keystroke.m_aCond[cond_type]);
				}
			}
			return TRUE;
		
		// Command
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				
				case IDCCHK_DISTINGUISH_LEFT_RIGHT:
					s_keystroke.m_sided = toBool(IsDlgButtonChecked(hDlg, IDCCHK_DISTINGUISH_LEFT_RIGHT));
					PostMessage(GetDlgItem(hDlg, IDCTXT), WM_KEYSTROKE, 0,0);
					break;
				
				case IDOK:
					{
						UINT idErrorString;
						
						if (!s_keystroke.m_vk) {
							idErrorString = ERR_INVALID_SHORTCUT;
							messageBox(hDlg, idErrorString, MB_ICONEXCLAMATION);
							SetFocus(GetDlgItem(hDlg, IDCTXT));
							break;
						}
						
						for (int i = 0; i < condTypeCount; i++) {
							s_keystroke.m_aCond[i] = ComboBox_GetCurSel(GetDlgItem(hDlg, IDCCBO_CAPSLOCK + i));
						}
					}
					// Fall-through
				
				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
					break;
			}
			break;
	}
	
	return 0;
}


// Edit text of keystroke dialog box
LRESULT CALLBACK prcKeystrokeCtl(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	bool bIsDown = false;
	
	switch (message) {
		
		case WM_LBUTTONDOWN:
			SetFocus(hWnd);
			break;
		
		case WM_SETCURSOR:
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			return TRUE;
		
		case WM_GETDLGCODE:
			return DLGC_WANTALLKEYS;
		
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (s_reset_keystroke) {
				s_keystroke.reset();
			}
			bIsDown = true;
			// Fall-through
		
		case WM_KEYUP:
		case WM_SYSKEYUP:
			{
				BYTE vk_typed = static_cast<BYTE>(wParam);
				// Flags
				DWORD sided_mod_code = s_keystroke.m_sided_mod_code;
				bool is_special = false;
				for (int i = 0; i < arrayLength(e_special_keys); i++) {
					const SpecialKey& special_key = e_special_keys[i];
					if ((vk_typed == special_key.vk) ||
					    (vk_typed == special_key.vk_left) ||
					    (vk_typed == special_key.vk_right)) {
						vk_typed = special_key.vk;
						is_special = true;
					}
					
					if (isKeyDown(special_key.vk_left)) {
						sided_mod_code |= special_key.mod_code;
					} else if (isKeyDown(special_key.vk_right)) {
						sided_mod_code |= special_key.mod_code << kRightModCodeOffset;
					} else {
						sided_mod_code &= ~special_key.mod_code;
					}
				}
				
				// Key
				// If key is pressed, update the flags
				// If key is released, update the flags only if no key has been set
				bool update_mode_code = true;
				if (bIsDown) {
					s_reset_keystroke = false;
					if (!is_special) {
						s_keystroke.m_vk = Keystroke::filterVK(vk_typed);
					}
				} else {
					s_reset_keystroke |= !is_special;
					update_mode_code &= !s_keystroke.m_vk;
				}
				if (update_mode_code) {
					s_keystroke.m_sided_mod_code = sided_mod_code;
				}
			}
			// Fall-through
		
		// Display keystroke name
		case WM_KEYSTROKE:
			{
				TCHAR pszHotKey[bufHotKey];
				s_keystroke.getKeyName(pszHotKey);
				SetWindowText(hWnd, pszHotKey);
				const int len = lstrlen(pszHotKey);
				Edit_SetSel(hWnd, len,len);
			}
			return 0;
	}
	
	// Ignore mouse and keyboard events
	if ((WM_MOUSEFIRST <= message && message <= WM_MOUSELAST)
			|| (WM_KEYFIRST <= message && message <= WM_KEYLAST)) {
		return 0;
	}
	
	return CallWindowProc(s_prcKeystrokeCtl, hWnd, message, wParam, lParam);
}


bool askKeystroke(HWND hwnd_parent, Keystroke* edited_keystroke, Keystroke& result_keystroke) {
	s_old_edited_keystroke = edited_keystroke;
	s_reset_keystroke = true;
	
	s_keystroke = result_keystroke;
	VERIF(IDOK == i18n::dialogBox(IDD_KEYSTROKE, hwnd_parent, prcKeystroke));
	
	result_keystroke = s_keystroke;
	return true;
}


//------------------------------------------------------------------------
// Keystroke
//------------------------------------------------------------------------

// Get the human readable name of the keystroke
void Keystroke::getKeyName(LPTSTR pszHotKey) const {
	*pszHotKey = _T('\0');
	
	// Special keys
	for (int i = 0; i < arrayLength(e_special_keys); i++) {
		const int mod_code_left = e_special_keys[i].mod_code;
		const int mod_code_right = mod_code_left << kRightModCodeOffset;
		if (m_sided_mod_code & (mod_code_left | mod_code_right)) {
			StrNCat(pszHotKey, getToken(e_special_keys[i].tok), bufHotKey);
			if (m_sided) {
				StrNCat(pszHotKey, _T(" "), bufHotKey);
				StrNCat(pszHotKey, getToken((m_sided_mod_code & mod_code_left) ? tokLeft : tokRight),
						bufHotKey);
			}
			StrNCat(pszHotKey, _T(" + "), bufHotKey);
		}
	}
	
	// Main key
	getKeyName(m_vk, pszHotKey);
}


bool Keystroke::isKeyExtended(UINT vk) {
	return vk == VK_NUMLOCK || vk == VK_DIVIDE || (vk >= VK_PRIOR && vk <= VK_DELETE);
}

// Get a key name and add it to pszHotKey
void Keystroke::getKeyName(UINT vk, LPTSTR pszHotKey) {
	long lScan = (::MapVirtualKey(LOBYTE(vk), 0) << 16) | (1L << 25);
	if (isKeyExtended(vk)) {
		lScan |= 1L << 24;
	}
	
	const int len = lstrlen(pszHotKey);
	::GetKeyNameText(lScan, pszHotKey + len, bufHotKey - len);
	if (vk >= 0xA6 && vk <= 0xB7 || (vk && !*pszHotKey)) {
		wsprintf(pszHotKey + len, _T("#%d"), vk);
	}
}


// Test for inclusion: is 'ks' a particular case of 'this'?
bool Keystroke::match(const Keystroke& ks) const {
	VERIF(m_vk == ks.m_vk);
	
	if (m_sided) {
		VERIF(ks.m_sided && m_sided_mod_code == ks.m_sided_mod_code);
	} else {
		VERIF(getUnsidedModCode() == ks.getUnsidedModCode());
	}
	
	for (int i = 0; i < condTypeCount; i++) {
		VERIF(m_aCond[i] == condIgnore || m_aCond[i] == ks.m_aCond[i]);
	}
	return true;
}


void Keystroke::registerHotKey() {
	const WORD mod_code = getUnsidedModCode();
	if (m_vk == VK_NUMPAD5) {
		RegisterHotKey(NULL, MAKEWORD(VK_CLEAR, mod_code), mod_code, VK_CLEAR);
	}
	RegisterHotKey(NULL, MAKEWORD(m_vk, mod_code), mod_code, m_vk);
}

bool Keystroke::unregisterHotKey() {
	const WORD mod_code = getUnsidedModCode();
	if (m_vk == VK_NUMPAD5) {
		UnregisterHotKey(NULL, MAKEWORD(VK_CLEAR, mod_code));
	}
	return toBool(UnregisterHotKey(NULL, MAKEWORD(m_vk, mod_code)));
}


// Serialize the shortcut itself (the key name)
void Keystroke::serialize(LPTSTR psz) {
	bool bSkipPlus = false;
	int mod_code_last = 0;
	
	for (;;) {
		
		// Skip spaces and '+'
		while (*psz == _T(' ') || (*psz == _T('+') && bSkipPlus)) {
			if (*psz == _T('+')) {
				mod_code_last = 0;
				bSkipPlus = false;
			}
			psz++;
		}
		bSkipPlus = true;
		if (!*psz) {
			break;
		}
		
		// Get the next word
		TCHAR *pcNext = psz;
		while (*pcNext && *pcNext != _T(' ') && *pcNext != _T('+')) {
			pcNext++;
		}
		const TCHAR cOldNext = *pcNext;
		*pcNext = _T('\0');
		
		
		const int tok = findToken(psz);
		
		if (tokWin <= tok && tok <= tokAlt) {
			// Special key token
			
			mod_code_last = 0;
			for (int i = 0; i < arrayLength(e_special_keys); i++) {
				if (e_special_keys[i].tok == tok) {
					mod_code_last = e_special_keys[i].mod_code;
					m_sided_mod_code |= mod_code_last;
					break;
				}
			}
			
		} else if (mod_code_last && (tok == tokLeft || tok == tokRight)) {
			// Key side
			
			if (tok == tokRight) {
				m_sided_mod_code &= ~mod_code_last;
				m_sided_mod_code |= mod_code_last << kRightModCodeOffset;
				mod_code_last = 0;
			}
			
		} else {
			// Normal key token
			
			*pcNext = cOldNext;
			
			for (int vk = 0x07; vk < 0xFF; vk++) {
				if (vk == 0x0A) {
					vk = 0x0B;
				} else if (vk == 0x5E || vk == 0xE0) {
					continue;
				} else if (vk == 0xB8) {
					vk = 0xB9;
				} else if (vk == 0xC1) {
					vk = 0xD7;
				} else if (vk == 0xA0) {
					vk = 0xA5;
				} else {
					TCHAR pszHotKey[bufHotKey] = _T("");
					getKeyName(vk, pszHotKey);
					if (!lstrcmpi(psz, pszHotKey)) {
						m_vk = filterVK((BYTE)vk);
						break;
					}
				}
			}
			
			// Stop here
			break;
		}
		
		psz = pcNext;
		if (cOldNext) {
			psz++;
		}
	}
}


void Keystroke::keybdEvent(UINT vk, bool down) {
	DWORD dwFlags = down ? 0 : KEYEVENTF_KEYUP;
	if (isKeyExtended(vk)) {
		dwFlags |= KEYEVENTF_EXTENDEDKEY;
	}
	keybd_event((BYTE)vk, (BYTE)MapVirtualKey(vk, 0), dwFlags, 0);
}


// Simulate the typing of the keystroke
void Keystroke::simulateTyping(HWND UNUSED(hwndFocus), bool bSpecialKeys) const {
	const DWORD mod_code = getUnsidedModCode();
	
	// Press the special keys
	if (bSpecialKeys) {
		for (int i = 0; i < arrayLength(e_special_keys); i++) {
			if (mod_code & e_special_keys[i].mod_code) {
				keybdEvent(e_special_keys[i].vk, true /* down */);
			}
		}
	}
	
	// Press and release the main key
	keybdEvent(m_vk, true /* down */);
	sleepBackground(0);
	keybdEvent(m_vk, false /* down */);
	
	// Release the special keys
	if (bSpecialKeys) {
		for (int i = 0; i < arrayLength(e_special_keys); i++) {
			if (mod_code & e_special_keys[i].mod_code) {
				keybdEvent(e_special_keys[i].vk, false /* down */);
			}
		}
	}
}


// Get the keyboard focus window
HWND Keystroke::getKeyboardFocus() {
	HWND hwndFocus;
	DWORD idThread;
	catchKeyboardFocus(hwndFocus, idThread);
	detachKeyboardFocus(idThread);
	return hwndFocus;
}

// Get the keyboard focus and the focus window
void Keystroke::catchKeyboardFocus(HWND& rhwndFocus, DWORD& ridThread) {
	const HWND hwndForeground = GetForegroundWindow();
	ridThread = GetWindowThreadProcessId(hwndForeground, NULL);
	AttachThreadInput(GetCurrentThreadId(), ridThread, TRUE);
	const HWND hwndFocus = GetFocus();
	rhwndFocus = (hwndFocus) ? hwndFocus : hwndForeground;
}

void Keystroke::detachKeyboardFocus(DWORD idThread) {
	AttachThreadInput(GetCurrentThreadId(), idThread, FALSE);
}

void Keystroke::releaseSpecialKeys(BYTE abKeyboard[]) {
	for (int i = 0; i < arrayLength(e_special_keys); i++) {
		const SpecialKey& special_key = e_special_keys[i];
		releaseSpecialKey(special_key.vk_left, abKeyboard);
		releaseSpecialKey(special_key.vk_right, abKeyboard);
		abKeyboard[special_key.vk] = 0;
	}
}

void Keystroke::releaseSpecialKey(BYTE vk, BYTE abKeyboard[]) {
	if (abKeyboard[vk] & keyDownMask) {
		abKeyboard[vk] = 0;
		keybdEvent(vk, false /* down */);
	}
}


//------------------------------------------------------------------------
// Comparison
//------------------------------------------------------------------------

int Keystroke::compare(const Keystroke& keystroke1, const Keystroke& keystroke2) {
	// Compare virtual key codes
	const int vk1 = static_cast<int>(keystroke1.m_vk);
	const int vk2 = static_cast<int>(keystroke2.m_vk);
	if (vk1 != vk2) {
		return vk1 - vk2;
	}
	
	// Compare special keys, applying the following order:
	// X
	// Win + X
	// Win + Ctrl + X
	// Win + Ctrl + Shift + X
	// Win + Shift + X
	// Win Left + Shift Left + X
	// Win Left + Shift Right + X
	// Win Right + Shift Left + X
	// Win Right + Shift Right + X
	// Ctrl + X
	// Ctrl + Shift + X
	// Ctrl + Shift + Alt + X
	// Ctrl + Alt + X
	// Ctrl Left + X
	// Shift + X
	// Alt + X
	DWORD remaining_mod_codes_mask = ~0U;
	for (int special_key = 0; special_key < arrayLength(e_special_keys); special_key++) {
		const int mod_code = e_special_keys[special_key].mod_code;
		remaining_mod_codes_mask &= ~(mod_code | (mod_code << kRightModCodeOffset));
		
		const int sort_key1 = keystroke1.getModCodeSortKey(mod_code, remaining_mod_codes_mask);
		const int sort_key2 = keystroke2.getModCodeSortKey(mod_code, remaining_mod_codes_mask);
		if (sort_key1 != sort_key2) {
			return sort_key1 - sort_key2;
		}
	}
	
	return 0;
}


int Keystroke::getModCodeSortKey(int mod_code, DWORD remaining_mod_codes_mask) const {
	if (m_sided) {
		if (m_sided_mod_code & mod_code) {
			// Left
			return 2;
		} else if (m_sided_mod_code & (mod_code << kRightModCodeOffset)) {
			// Right
			return 3;
		}
	} else if (m_sided_mod_code & mod_code) {
		// Unsided
		return 1;
	}
	
	// None
	return (m_sided_mod_code & remaining_mod_codes_mask) ? 4 : 0;
}


//------------------------------------------------------------------------
// Settings serialization
//------------------------------------------------------------------------

int e_acxCol[colCount];

SIZE e_sizeMainDialog = { 0, 0 };
bool e_bMaximizeMainDialog = false;

TCHAR e_pszIniFile[MAX_PATH];


bool Keystroke::askSendKeys(HWND hwnd_parent, Keystroke& rks) {
	s_keystroke.reset();
	s_keystroke.m_sided = false;
	
	VERIF(IDOK == i18n::dialogBox(IDD_SENDKEYS, hwnd_parent, prcSendKeys));
	
	rks = s_keystroke;
	return true;
}


INT_PTR CALLBACK Keystroke::prcSendKeys(HWND hDlg, UINT message, WPARAM wParam, LPARAM) {
	switch (message) {
		
		// Initialization
		case WM_INITDIALOG:
			{
				e_hdlgModal = hDlg;
				centerParent(hDlg);
				
				const HWND hctl = GetDlgItem(hDlg, IDCTXT);
				s_prcKeystrokeCtl = subclassWindow(hctl, prcKeystrokeCtl);
				PostMessage(hctl, WM_KEYSTROKE, 0,0);
			}
			return TRUE;
		
		// Command
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
					break;
			}
			break;
	}
	
	return 0;
}
