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
#include "Hook.h"
#include "Keystroke.h"


const SPECIALKEY e_aSpecialKey[nbSpecialKey] = {
	{ VK_LWIN, VK_LWIN, MOD_WIN, tokWin },
	{ VK_CONTROL, VK_LCONTROL, MOD_CONTROL, tokCtrl },
	{ VK_SHIFT, VK_LSHIFT, MOD_SHIFT, tokShift },
	{ VK_MENU, VK_LMENU, MOD_ALT, tokAlt },
};


// Pointer to the old value of the currently edited keystroke
static Keystroke *s_pksEditedOld;

// TRUE if we should reset the keystroke textbox ASAP
static bool s_ksReset;

// Currently edited keystroke value
static Keystroke s_ks;

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
				keyboard_hook::setCatchAllKeysWindow(hwnd_keystroke);
				PostMessage(hwnd_keystroke, WM_KEYSTROKE, 0,0);
				
				CheckDlgButton(hDlg, IDCCHK_DISTINGUISH_LEFT_RIGHT, s_ks.m_sided);
				
				// Load condition names
				TCHAR pszConds[bufString];
				i18n::loadStringAuto(IDS_CONDITIONS, pszConds);
				TCHAR *pc = pszConds;
				for (int iCond = 0; iCond < condCount; iCond++) {
					while (*pc != _T(';')) {
						pc++;
					}
					*pc++ = _T('\0');
				}
				
				for (int i = 0; i < condTypeCount; i++) {
					const HWND hcbo = GetDlgItem(hDlg, IDCCBO_CAPSLOCK + i);
					pc = pszConds;
					for (int j = 0; j < condCount; j++) {
						ComboBox_AddString(hcbo, pc);
						pc += lstrlen(pc) + 1;
					}
					ComboBox_SetCurSel(hcbo, s_ks.m_aCond[i]);
				}
			}
			return TRUE;
		
		// Command
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				
				case IDCCHK_DISTINGUISH_LEFT_RIGHT:
					s_ks.m_sided = toBool(IsDlgButtonChecked(hDlg, IDCCHK_DISTINGUISH_LEFT_RIGHT));
					PostMessage(GetDlgItem(hDlg, IDCTXT), WM_KEYSTROKE, 0,0);
					break;
				
				case IDOK:
					{
						UINT idErrorString;
						
						if (!s_ks.m_vk) {
							idErrorString = ERR_INVALID_SHORTCUT;
							messageBox(hDlg, idErrorString, MB_ICONEXCLAMATION);
							SetFocus(GetDlgItem(hDlg, IDCTXT));
							break;
						}
						
						for (int i = 0; i < condTypeCount; i++) {
							s_ks.m_aCond[i] = ComboBox_GetCurSel(GetDlgItem(hDlg, IDCCBO_CAPSLOCK + i));
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
		
		case WM_DESTROY:
			keyboard_hook::setCatchAllKeysWindow(NULL);
			break;
		
		case WM_LBUTTONDOWN:
			SetFocus(hWnd);
			break;
		
		case WM_SETCURSOR:
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			return TRUE;
		
		case WM_GETDLGCODE:
			return DLGC_WANTALLKEYS;
		
		// These messages are sent by the keyboard hook. Arguments:
		// wParam: the virtual key code
		// lParam: the special keys bitmask
		//   special_key_index * 2: "left key is down" bit
		//   special_key_index * 2 + 1: "right key is down" bit
		case WM_CLAVIER_KEYDOWN:
			if (s_ksReset) {
				s_ks.reset();
			}
			bIsDown = true;
			// Fall-through
		
		case WM_CLAVIER_KEYUP:
			{
				BYTE vk_typed = static_cast<BYTE>(wParam);
				const DWORD special_keys_down_mask = static_cast<DWORD>(lParam);
				
				// Flags
				DWORD sided_mod_code = s_ks.m_sided_mod_code;
				bool bIsSpecial = false;
				for (int special_key = 0; special_key < arrayLength(e_aSpecialKey); special_key++) {
					const SPECIALKEY& rspecial_key = e_aSpecialKey[special_key];
					
					if (vk_typed == rspecial_key.vk || vk_typed == rspecial_key.vk_left
							|| vk_typed == getRightVkFromLeft(rspecial_key.vk_left)) {
						vk_typed = rspecial_key.vk;
						bIsSpecial = true;
					}
					
					const DWORD mod_code = rspecial_key.mod_code;
					if (special_keys_down_mask & (1L << (special_key * 2))) {
						sided_mod_code |= mod_code;
					} else if (special_keys_down_mask & (1L << (special_key * 2 + 1))) {
						sided_mod_code |= mod_code << kRightModCodeOffset;
					} else {
						sided_mod_code &= ~mod_code;
					}
				}
				
				// Key
				// If key is pressed, update the flags
				// If key is released, update the flags only if no key has been set
				bool update_mode_code = true;
				if (bIsDown) {
					s_ksReset = false;
					if (!bIsSpecial) {
						s_ks.m_vk = Keystroke::filterVK(vk_typed);
					}
				} else {
					s_ksReset |= !bIsSpecial;
					update_mode_code &= !s_ks.m_vk;
				}
				if (update_mode_code) {
					s_ks.m_sided_mod_code = sided_mod_code;
				}
			}
			// Fall-through
		
		// Display keystroke name
		case WM_KEYSTROKE:
			{
				TCHAR pszHotKey[bufHotKey];
				s_ks.getKeyName(pszHotKey);
				if (!*pszHotKey) {
					lstrcpy(pszHotKey, getToken(tokNone));
				}
				
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


bool askKeystroke(HWND hwnd_parent, Keystroke* pksEdited, Keystroke& rksResult) {
	s_pksEditedOld = pksEdited;
	s_ksReset = true;
	
	s_ks = rksResult;
	VERIF(IDOK == i18n::dialogBox(IDD_KEYSTROKE, hwnd_parent, prcKeystroke));
	
	rksResult = s_ks;
	return true;
}


//------------------------------------------------------------------------
// Keystroke
//------------------------------------------------------------------------

// Get the human readable name of the keystroke
void Keystroke::getKeyName(LPTSTR pszHotKey) const {
	*pszHotKey = _T('\0');
	
	// Special keys
	for (int i = 0; i < arrayLength(e_aSpecialKey); i++) {
		const int mod_code_left = e_aSpecialKey[i].mod_code;
		const int mod_code_right = mod_code_left << kRightModCodeOffset;
		if (m_sided_mod_code & (mod_code_left | mod_code_right)) {
			StrNCat(pszHotKey, getToken(e_aSpecialKey[i].tok), bufHotKey);
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
			for (int special_key = 0; special_key < arrayLength(e_aSpecialKey); special_key++) {
				if (e_aSpecialKey[special_key].tok == tok) {
					mod_code_last = e_aSpecialKey[special_key].mod_code;
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


void Keystroke::keybdEvent(UINT vk, bool bUp) {
	DWORD dwFlags = (bUp) ? KEYEVENTF_KEYUP : 0;
	if (isKeyExtended(vk)) {
		dwFlags |= KEYEVENTF_EXTENDEDKEY;
	}
	keybd_event((BYTE)vk, (BYTE)MapVirtualKey(vk, 0), dwFlags, 0);
}


// Simulate the typing of the keystroke
void Keystroke::simulateTyping(HWND MY_UNUSED(hwndFocus), bool bSpecialKeys) const {
	const DWORD mod_code = getUnsidedModCode();
	
	// Press the special keys
	if (bSpecialKeys) {
		for (int special_key = 0; special_key < arrayLength(e_aSpecialKey); special_key++) {
			if (mod_code & e_aSpecialKey[special_key].mod_code) {
				keybdEvent(e_aSpecialKey[special_key].vk, false);
			}
		}
	}
	
	// Press and release the main key
	keybdEvent(m_vk, false);
	sleepBackground(0);
	keybdEvent(m_vk, true);
	
	// Release the special keys
	if (bSpecialKeys) {
		for (int special_key = 0; special_key < arrayLength(e_aSpecialKey); special_key++) {
			if (mod_code & e_aSpecialKey[special_key].mod_code) {
				keybdEvent(e_aSpecialKey[special_key].vk, true);
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
	for (int special_key = 0; special_key < arrayLength(e_aSpecialKey); special_key++) {
		const BYTE vk = e_aSpecialKey[special_key].vk;
		if (abKeyboard[vk] & keyDownMask) {
			abKeyboard[vk] = 0;
			keybdEvent(vk, true);
		}
	}
}


//------------------------------------------------------------------------
// Comparison
//------------------------------------------------------------------------

int Keystroke::compare(const Keystroke& keystroke1, const Keystroke& keystroke2) {
	
	// Compare special keys, applying the following order:
	// X
	// Win + X
	// Win + Ctrl + X
	// Win Left + Shift Left + X
	// Win Left + Shift Right + X
	// Win Right + Shift Left + X
	// Win Right + Shift Right + X
	// Ctrl + X
	// Ctrl + Shift + X
	// Ctrl + Shift + Alt + X
	// Ctrl + Alt + X
	// Ctrl Left + X
	int index1 = 0;
	int index2 = 0;
	int inherited1 = 0;
	int inherited2 = 0;
	int offset = 0;
	for (int special_key = arrayLength(e_aSpecialKey) - 1; special_key >= 0; special_key--) {
		const int mod_code = e_aSpecialKey[special_key].mod_code;
		const int value1 = keystroke1.getComparableSpecialKey(mod_code, inherited1);
		const int value2 = keystroke2.getComparableSpecialKey(mod_code, inherited2);
		
		if (value1) {
			inherited1 = 7;
		}
		if (value2) {
			inherited2 = 7;
		}
		
		index1 += value1 << offset;
		index2 += value2 << offset;
		offset += 3;
	}
	
	const int diff = index1 - index2;
	if (diff) {
		return diff;
	}
	
	// Compare virtual key codes
	return static_cast<int>(keystroke1.m_vk) - static_cast<int>(keystroke2.m_vk);
}


int Keystroke::getComparableSpecialKey(int mod_code, int if_none) const {
	if (m_sided) {
		if (m_sided_mod_code & mod_code) {
			// Left
			return 2;
		} else if (m_sided_mod_code & (mod_code << kRightModCodeOffset)) {
			// Right
			return 3;
		} else {
			// Sided, none
			return if_none;
		}
	} else {
		if (m_sided_mod_code & mod_code) {
			// Unsided
			return 1;
		} else {
			// Unsided, none
			return if_none;
		}
	}
}


//------------------------------------------------------------------------
// Settings serialization
//------------------------------------------------------------------------

int e_acxCol[colCount];

SIZE e_sizeMainDialog = { 0, 0 };
bool e_bMaximizeMainDialog = false;

TCHAR e_pszIniFile[MAX_PATH];


// Open the key for Clavier+ launching at Windows startup
HKEY openAutoStartKey(LPTSTR pszPath) {
	if (!GetModuleFileName(NULL, pszPath, MAX_PATH)) {
		*pszPath = 0;
	}
	HKEY hKey;
	if (ERROR_SUCCESS != RegOpenKey(HKEY_CURRENT_USER,
			_T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), &hKey)) {
		hKey = NULL;
	}
	return hKey;
}


bool Keystroke::askSendKeys(HWND hwnd_parent, Keystroke& rks) {
	s_ks.reset();
	s_ks.m_sided = false;
	
	VERIF(IDOK == i18n::dialogBox(IDD_SENDKEYS, hwnd_parent, prcSendKeys));
	
	rks = s_ks;
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
