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


#pragma once

#include "App.h"

// Indicates if a key was down when the last thread message was received.
inline bool isKeyDown(UINT vk) {
	return toBool(GetKeyState(vk) & 0x8000);
}

// Indicates if a key is currently down.
inline bool isAsyncKeyDown(UINT vk) {
	return toBool(GetAsyncKeyState(vk) & 0x8000);
}

const BYTE keyDownMask = 0x80;

const int kRightModCodeOffset = 16;


enum {
	condTypeShiftLock,
	condTypeNumLock,
	condTypeScrollLock,
	condTypeCount
};

enum {
	condIgnore,
	condYes,
	condNo,
	condCount
};


class Keystroke {
public:
	
	// Virtual key code of the keystroke
	BYTE m_vk;
	
	// Bitmask of the special keys mode codes of this shortcut.
	// Bits 0..kRightModCodeOffset-1 are for the left special keys.
	// Bits kRightModCodeOffset..2*kRightModCodeOffset-1 are for the right special keys.
	// If m_sided is false, left and right special keys are not distinguished and only bits
	// 0..kRightModCodeOffset-1 are taken into account.
	DWORD m_sided_mod_code;
	
	int m_aCond[condTypeCount];
	
	// True if m_sided_mod_code distinguishes between left and right special keys.
	bool m_sided;
	
public:
	
	Keystroke() {
		resetAll();
	}
	
	void reset() {
		m_vk = 0;
		m_sided_mod_code = 0;
	}
	
	void resetAll() {
		reset();
		m_sided = false;
		for (int i = 0; i < condTypeCount; i++) {
			m_aCond[i] = condIgnore;
		}
	}
	
	
	WORD getUnsidedModCode() const {
		return (WORD)m_sided_mod_code | (WORD)(m_sided_mod_code >> kRightModCodeOffset);
	}
	
	void getKeyName(LPTSTR pszHotKey) const;
	
	void serialize(LPTSTR psz);
	void simulateTyping(HWND hwndFocus, bool bSpecialKeys = true) const;
	
	void registerHotKey();
	bool unregisterHotKey();
	
	bool match(const Keystroke& ks) const;
	
	bool canReleaseSpecialKeys() const {
		return (VK_BROWSER_BACK > m_vk) ||  // first media key
		       (m_vk > VK_LAUNCH_APP2);     // last media key
	}
	
	static bool askSendKeys(HWND hwnd_parent, Keystroke& rks);
	
	
	static void keybdEvent(UINT vk, bool down);
	static BYTE filterVK(BYTE vk) {
		return (vk == VK_CLEAR) ? VK_NUMPAD5 : vk;
	}
	static bool isKeyExtended(UINT vk);
	static void getKeyName(UINT vk, LPTSTR pszHotKey);
	static HWND getKeyboardFocus();
	static void catchKeyboardFocus(HWND& rhwndFocus, DWORD& ridThread);
	static void detachKeyboardFocus(DWORD idThread);
	static void releaseSpecialKeys(BYTE abKeyboard[]);
	
	// Compares two keystrokes.
	//
	// Returns:
	//   A < 0 integer if keystroke1 is before keystroke2, > 0 if keystroke1 is after keystroke2,
	//   0 if the keystrokes are equal.
	static int compare(const Keystroke& keystroke1, const Keystroke& keystroke2);
	
	// Returns an integer between 0 and 3 inclusive to be used for comparing the mode codes of two
	// keystrokes for a given special key. Mod codes order: none < unsided < left < right.
	//
	// Args:
	//   mod_code: Unsided MOD_* constant identifying the special key.
	//   if_none: Value to return if the keystroke does not use the given special key.
	//
	// Returns:
	//   - if_none if the keystroke does not use the special key.
	//   - 1 if the keystroke is unsided and uses the given special key.
	//   - 2 if the keystroke is sided and uses the given special key as left.
	//   - 3 if the keystroke is sided and uses the given special key as right.
	int getComparableSpecialKey(int mod_code, int if_none) const;
	
private:
	
	static INT_PTR CALLBACK prcSendKeys(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};


struct SpecialKey {
	BYTE vk;  // Virtual key code, undistinguishable from left and right if possible
	BYTE vk_left;  // Virtual code of the left key
	BYTE vk_right;  // Virtual code of the right key
	DWORD mod_code;  // MOD_* code of the key (e.g. MOD_CONTROL), not sided
	int tok;  // Token index of the name of the key
};

const int kSpecialKeysCount = 4;
extern const SpecialKey e_special_keys[kSpecialKeysCount];


bool askKeystroke(HWND hwnd_parent, Keystroke* edited_keystroke, Keystroke& result_keystroke);
