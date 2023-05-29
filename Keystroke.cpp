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


String Keystroke::s_vk_key_names[256];
BYTE Keystroke::s_next_named_vk[256];


void Keystroke::loadVkKeyNames() {
	BYTE last_named_vk = 0xFF;
	for (int vk = last_named_vk - 1; vk >= 0; vk--) {
		String* key_name = &s_vk_key_names[vk];
		loadVkKeyName(BYTE(vk), key_name);
		s_next_named_vk[vk] = last_named_vk;
		if (key_name->isSome()) {
			last_named_vk = BYTE(vk);
		}
	}
}

void Keystroke::loadVkKeyName(BYTE vk, String* output) {
	if (vk <= 0x07
			|| (0x0A <= vk && vk <= 0x0B)  // reserved
			|| vk == 0x5E  // reserved
			|| (0x88 <= vk && vk <= 0x8F)  // reserved
			|| (VK_LSHIFT <= vk && vk <= VK_RMENU)  // special keys
			|| (0xB8 <= vk && vk <= 0xB9)  // reserved
			|| (0xC1 <= vk && vk <= 0xC2)  // reserved
			|| (0xC3 <= vk && vk <= 0xDA)  // gamepad input
			|| vk == 0xE0  // reserved
			|| vk == 0xFF) {  // reserved & sentinel
		return;
	}
	
	// GetKeyNameText bug: support VK_F13..VK_F24 manually.
	if (VK_F1 <= vk && vk <= VK_F24) {
		wsprintf(output->getBuffer(4), _T("F%d"), vk - VK_F1 + 1);
		return;
	}
	
	// Retrieve the key name with GetKeyNameText.
	// Do not distinguish between left and right.
	// Fallback to a numerical name like #123 on error,
	// for media keys (they have invalid key codes), and for some extended keys
	// (to avoid ambiguities like VK_OEM_5 and VK_OEM_102 both named "\").
	UINT scan_code = MapVirtualKey(LOBYTE(vk), MAPVK_VK_TO_VSC);
	long lParam = (scan_code << 16) | (1L << 25);
	if (isKeyExtended(vk)) {
		lParam |= 1L << 24;
	}
	if ((VK_BROWSER_BACK <= vk && vk <= VK_LAUNCH_APP2) || (vk >= 0xE1) ||
			!GetKeyNameText(lParam, output->getBuffer(kHotKeyBufSize), kHotKeyBufSize)) {
		wsprintf(output->getBuffer(kHotKeyBufSize), _T("#%d"), vk);
	}
}



void Keystroke::getDisplayName(LPTSTR output) const {
	*output = _T('\0');
	
	// Special keys
	for (const auto& special_key : kSpecialKeys) {
		const int mod_code_left = special_key.mod_code;
		const int mod_code_right = mod_code_left << kRightModCodeOffset;
		if (m_sided_mod_code & (mod_code_left | mod_code_right)) {
			StringCchCat(output, kHotKeyBufSize, getToken(special_key.tok));
			if (m_sided) {
				StringCchCat(output, kHotKeyBufSize, _T(" "));
				StringCchCat(
					output, kHotKeyBufSize,
					getToken((m_sided_mod_code & mod_code_left) ? Token::kLeft : Token::kRight));
			}
			StringCchCat(output, kHotKeyBufSize, _T(" + "));
		}
	}
	
	// Main key
	StringCchCat(output, kHotKeyBufSize, getKeyName(m_vk));
}

void Keystroke::parseDisplayName(LPTSTR input) {
	bool skip_plus = false;
	int mod_code_last = 0;
	
	for (;;) {
		
		// Skip spaces and '+'
		while (*input == _T(' ') || (*input == _T('+') && skip_plus)) {
			if (*input == _T('+')) {
				mod_code_last = 0;
				skip_plus = false;
			}
			input++;
		}
		skip_plus = true;
		if (!*input) {
			break;
		}
		
		// Get the next word
		TCHAR *next_word = input;
		while (*next_word && *next_word != _T(' ') && *next_word != _T('+')) {
			next_word++;
		}
		const TCHAR next_word_copy = *next_word;
		*next_word = _T('\0');
		
		const Token tok = findToken(input);
		
		if (Token::kWin <= tok && tok <= Token::kAlt) {
			// Special key token
			
			mod_code_last = 0;
			for (const auto& special_key : kSpecialKeys) {
				if (special_key.tok == tok) {
					mod_code_last = special_key.mod_code;
					m_sided_mod_code |= mod_code_last;
					break;
				}
			}
			
		} else if (mod_code_last && (tok == Token::kLeft || tok == Token::kRight)) {
			// Key side
			
			m_sided = true;
			if (tok == Token::kRight) {
				m_sided_mod_code &= ~mod_code_last;
				m_sided_mod_code |= mod_code_last << kRightModCodeOffset;
				mod_code_last = 0;
			}
			
		} else {
			// Normal key token
			
			*next_word = next_word_copy;
			
			if (!*input) {
				break;
			}
			for (BYTE vk = s_next_named_vk[0]; vk < 0xFF; vk = s_next_named_vk[vk]) {
				if (!lstrcmpi(input, s_vk_key_names[vk])) {
					m_vk = canonicalizeKey(vk);
					break;
				}
			}
			break;
		}
		
		input = next_word;
		if (next_word_copy) {
			input++;
		}
	}
}


void Keystroke::simulateTyping(DWORD already_down_mod_code) const {
	const bool was_registered = unregisterHotKey();
	const DWORD to_press_mod_code = getUnsidedModCode() & ~already_down_mod_code;
	
	// Press the special keys that are not already kept down.
	for (const auto& special_key : kSpecialKeys) {
		if (to_press_mod_code & special_key.mod_code) {
			keybdEvent(special_key.vk, /* down= */ true);
		}
	}
	
	// Press and release the main key.
	keybdEvent(m_vk, /* down= */ true);
	sleepBackground(0);
	keybdEvent(m_vk, /* down= */ false);
	
	// Release the special keys that should not be kept down.
	for (const auto& special_key : kSpecialKeys) {
		if (to_press_mod_code & special_key.mod_code) {
			keybdEvent(special_key.vk, /* down= */ false);
		}
	}
	
	if (was_registered) {
		registerHotKey();
	}
}

void Keystroke::keybdEvent(UINT vk, bool down) {
	DWORD dwFlags = down ? 0 : KEYEVENTF_KEYUP;
	if (isKeyExtended(vk)) {
		dwFlags |= KEYEVENTF_EXTENDEDKEY;
	}
	keybd_event(BYTE(vk), BYTE(MapVirtualKey(vk, 0)), dwFlags, 0);
}


void Keystroke::registerHotKey() const {
	const WORD mod_code = getUnsidedModCode();
	if (m_vk == VK_NUMPAD5) {
		RegisterHotKey(/* hWnd= */ NULL, MAKEWORD(VK_CLEAR, mod_code), mod_code, VK_CLEAR);
	}
	RegisterHotKey(/* hWnd= */ NULL, MAKEWORD(m_vk, mod_code), mod_code, m_vk);
}

bool Keystroke::unregisterHotKey() const {
	const WORD mod_code = getUnsidedModCode();
	if (m_vk == VK_NUMPAD5) {
		UnregisterHotKey(/* hWnd= */ NULL, MAKEWORD(VK_CLEAR, mod_code));
	}
	return toBool(UnregisterHotKey(/* hWnd= */ NULL, MAKEWORD(m_vk, mod_code)));
}


bool Keystroke::isKeyExtended(UINT vk) {
	// http://docs.microsoft.com/windows/win32/inputdev/about-keyboard-input#extended-key-flag
	return
		(VK_PRIOR <= vk && vk <= VK_DOWN) ||
		(VK_SNAPSHOT <= vk && vk <= VK_DELETE) ||
		vk == VK_DIVIDE || vk == VK_NUMLOCK;
}


bool Keystroke::isSubset(const Keystroke& other) const {
	VERIF(m_vk == other.m_vk);
	
	if (m_sided) {
		VERIF(other.m_sided && m_sided_mod_code == other.m_sided_mod_code);
	} else {
		VERIF(getUnsidedModCode() == other.getUnsidedModCode());
	}
	
	for (int i = 0; i < kCondTypeCount; i++) {
		VERIF(m_conditions[i] == Condition::kIgnore || m_conditions[i] == other.m_conditions[i]);
	}
	return true;
}


HWND Keystroke::getInputFocus() {
	HWND new_input_window;
	DWORD new_input_thread;
	catchKeyboardFocus(&new_input_window, &new_input_thread);
	detachKeyboardFocus(new_input_thread);
	return new_input_window;
}

void Keystroke::catchKeyboardFocus(HWND* new_input_window, DWORD* new_input_thread) {
	const HWND foreground_window = GetForegroundWindow();
	*new_input_thread = GetWindowThreadProcessId(foreground_window, /* lpdwProcessId= */ nullptr);
	AttachThreadInput(GetCurrentThreadId(), *new_input_thread, /* fAttach= */ true);
	const HWND input_window = GetFocus();
	*new_input_window = input_window ? input_window : foreground_window;
}

void Keystroke::detachKeyboardFocus(DWORD input_thread) {
	AttachThreadInput(GetCurrentThreadId(), input_thread, /* fAttach= */ false);
}

void Keystroke::resetKeyboardFocus(HWND* new_input_window, DWORD* input_thread) {
	detachKeyboardFocus(*input_thread);
	catchKeyboardFocus(new_input_window, input_thread);
}

void Keystroke::releaseSpecialKeys(BYTE keyboard_state[], DWORD keep_down_mod_code) {
	for (const auto& special_key : kSpecialKeys) {
		if (keep_down_mod_code & special_key.mod_code) {
			continue;
		}
		releaseKey(special_key.vk_left, keyboard_state);
		releaseKey(special_key.vk_right, keyboard_state);
		keyboard_state[special_key.vk] = 0;
	}
}

void Keystroke::releaseKey(BYTE vk, BYTE keyboard_state[]) {
	if (keyboard_state[vk] & kKeyDownMask) {
		keyboard_state[vk] = 0;
		keybdEvent(vk, /* down= */ false);
	}
}


int Keystroke::compare(const Keystroke& keystroke1, const Keystroke& keystroke2) {
	// Compare virtual key codes
	const int vk1 = int(keystroke1.m_vk);
	const int vk2 = int(keystroke2.m_vk);
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
	DWORD remaining_mod_codes_mask = kModCodeAll;
	for (const auto& special_key : kSpecialKeys) {
		const int mod_code = special_key.mod_code;
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


namespace {

// True if the currently edited keystroke should be reset ASAP.
// The keystroke is reset when new keys are pressed after all special keys have been released.
bool s_reset_keystroke;

// Currently edited keystroke
Keystroke s_keystroke;

// Keystroke control in prcEditDialog.
LRESULT CALLBACK prcKeystrokeCtl(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR UNUSED(subclass_id), DWORD_PTR UNUSED(ref_data)) {
	bool down = false;
	
	switch (message) {
		case WM_LBUTTONDOWN:
			SetFocus(hwnd);
			break;
		
		case WM_SETCURSOR:
			SetCursor(LoadCursor(/* hInstance= */ NULL, IDC_ARROW));
			return true;
		
		case WM_GETDLGCODE:
			return DLGC_WANTALLKEYS;
		
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (s_reset_keystroke) {
				s_keystroke.clearKeys();
			}
			down = true;
			// Fall-through
		
		case WM_KEYUP:
		case WM_SYSKEYUP: {
			BYTE vk_typed = BYTE(wParam);
			// Flags
			DWORD sided_mod_code = s_keystroke.m_sided_mod_code;
			bool is_special = false;
			for (const auto& special_key : kSpecialKeys) {
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
			if (down) {
				s_reset_keystroke = false;
				if (!is_special) {
					s_keystroke.m_vk = Keystroke::canonicalizeKey(vk_typed);
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
		case WM_KEYSTROKE: {
			TCHAR keystroke_display_name[kHotKeyBufSize];
			s_keystroke.getDisplayName(keystroke_display_name);
			SetWindowText(hwnd, keystroke_display_name);
			const int len = lstrlen(keystroke_display_name);
			Edit_SetSel(hwnd, len,len);
			return 0;
		}
	}
	
	// Ignore mouse and keyboard events
	if ((WM_MOUSEFIRST <= message && message <= WM_MOUSELAST)
			|| (WM_KEYFIRST <= message && message <= WM_KEYLAST)) {
		return 0;
	}
	
	return DefSubclassProc(hwnd, message, wParam, lParam);
}

INT_PTR CALLBACK prcEditDialog(HWND hdlg, UINT message, WPARAM wParam, LPARAM UNUSED(lParam)) {
	switch (message) {
		
		// Initialization
		case WM_INITDIALOG: {
			e_modal_dialog = hdlg;
			centerParent(hdlg);
				
			// Subclass the keystroke control. Display the initial keystroke name.
			const HWND hwnd_keystroke = GetDlgItem(hdlg, IDCTXT);
			subclassWindow(hwnd_keystroke, prcKeystrokeCtl);
			PostMessage(hwnd_keystroke, WM_KEYSTROKE, 0,0);
				
			CheckDlgButton(hdlg, IDCCHK_DISTINGUISH_LEFT_RIGHT, s_keystroke.m_sided);
				
			// Load ';'-separated condition names. Make them null-terminated strings.
			TCHAR conditions[kStringBufSize];
			i18n::loadStringAuto(IDS_CONDITIONS, conditions);
			TCHAR* current_condition = conditions;
			for (int cond = 0; cond < Keystroke::kConditionCount; cond++) {
				getSemiColonToken(&current_condition);
			}
				
			// Initialize the condition combo-boxes
			for (int cond_type = 0; cond_type < Keystroke::kCondTypeCount; cond_type++) {
				const HWND hwnd_cond_type = GetDlgItem(hdlg, IDCCBO_CAPSLOCK + cond_type);
				current_condition = conditions;
				for (int cond = 0; cond < Keystroke::kConditionCount; cond++) {
					ComboBox_AddString(hwnd_cond_type, current_condition);
					current_condition += lstrlen(current_condition) + 1;
				}
				ComboBox_SetCurSel(hwnd_cond_type, s_keystroke.m_conditions[cond_type]);
			}
			return true;
		}
		
		// Command
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				
				case IDCCHK_DISTINGUISH_LEFT_RIGHT:
					s_keystroke.m_sided = toBool(IsDlgButtonChecked(hdlg, IDCCHK_DISTINGUISH_LEFT_RIGHT));
					PostMessage(GetDlgItem(hdlg, IDCTXT), WM_KEYSTROKE, 0,0);
					break;
				
				case IDOK: {
					if (!s_keystroke.m_vk) {
						messageBox(hdlg, ERR_INVALID_SHORTCUT, MB_ICONEXCLAMATION);
						SetFocus(GetDlgItem(hdlg, IDCTXT));
						break;
					}
					
					for (int i = 0; i < Keystroke::kCondTypeCount; i++) {
						s_keystroke.m_conditions[i] =
							Keystroke::Condition(ComboBox_GetCurSel(GetDlgItem(hdlg, IDCCBO_CAPSLOCK + i)));
					}
				}
				// Fall-through
				
				case IDCANCEL:
					EndDialog(hdlg, LOWORD(wParam));
					break;
			}
			break;
	}
	
	return 0;
}

}  // namespace

bool Keystroke::showEditDialog(HWND hwnd_parent) {
	s_reset_keystroke = true;
	
	s_keystroke = *this;
	VERIF(IDOK == i18n::dialogBox(IDD_KEYSTROKE, hwnd_parent, prcEditDialog));
	
	*this = s_keystroke;
	return true;
}


namespace {

INT_PTR CALLBACK prcSendKeysDialog(HWND hdlg, UINT message, WPARAM wParam, LPARAM UNUSED(lParam)) {
	switch (message) {
		
		// Initialization
		case WM_INITDIALOG: {
			e_modal_dialog = hdlg;
			centerParent(hdlg);
				
			const HWND hctl = GetDlgItem(hdlg, IDCTXT);
			subclassWindow(hctl, prcKeystrokeCtl);
			PostMessage(hctl, WM_KEYSTROKE, 0,0);
			return true;
		}
		
		// Command
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
				case IDCANCEL:
					EndDialog(hdlg, LOWORD(wParam));
					break;
			}
			break;
	}
	
	return 0;
}

}  // namespace

bool Keystroke::showSendKeysDialog(HWND hwnd_parent) {
	s_keystroke.clearKeys();
	s_keystroke.m_sided = false;
	
	VERIF(IDOK == i18n::dialogBox(IDD_SENDKEYS, hwnd_parent, prcSendKeysDialog));
	
	*this = s_keystroke;
	return true;
}
