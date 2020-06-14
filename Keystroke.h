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

constexpr BYTE keyDownMask = 0x80;

constexpr int kRightModCodeOffset = 16;
constexpr DWORD kModCodeAll = ~0U;


enum KeystrokeConditionType {
	condTypeShiftLock,
	condTypeNumLock,
	condTypeScrollLock,
	condTypeCount
};

enum KeystrokeCondition {
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
	// Each bitset follows the MOD_* constants convention.
	// If m_sided is false, left and right special keys are not distinguished and only bits
	// 0..kRightModCodeOffset-1 are taken into account.
	DWORD m_sided_mod_code;
	
	// Indexed by KeystrokeConditionType.
	KeystrokeCondition m_conditions[condTypeCount];
	
	// True if m_sided_mod_code distinguishes between left and right special keys.
	bool m_sided;
	
public:
	
	Keystroke() {
		clear();
	}
	
	// Completely resets this keystroke to the empty state.
	void clear() {
		clearKeys();
		m_sided = false;
		for (int i = 0; i < condTypeCount; i++) {
			m_conditions[i] = condIgnore;
		}
	}
	
	// Resets the keys of this keystroke, but not the conditions or sided-ness.
	void clearKeys() {
		m_vk = 0;
		m_sided_mod_code = 0;
	}
	
	
	WORD getUnsidedModCode() const {
		return static_cast<WORD>(m_sided_mod_code) |
		       static_cast<WORD>(m_sided_mod_code >> kRightModCodeOffset);
	}
	
	// Gets the human readable name of this keystroke.
	void getDisplayName(LPTSTR output) const;
	
	// Appends the display name of a virtual key code to output.
	static void appendKeyName(UINT vk, LPTSTR output);
	
	// Parses the given human readable name. Accepts the output of getDisplayName().
	// May modify the input. Assumes the keystroke is initially cleared.
	void parseDisplayName(LPTSTR input);
	
	
	// Simulate typing the keystroke: presses the keys down and up.
	// Simulates special keys as unsided.
	// If the keystroke is registered as a global hotkey, unregisters it before simulating it
	// and re-registers it afterwards to avoid recursive calls.
	// special_keys: whether to simulate the special keys too.
	void simulateTyping(bool special_keys = true) const;
	
	// Simulates pressing or releasing a virtual key. Wrapper for keybd_event().
	static void keybdEvent(UINT vk, bool down);
	
	
	// Registers a global shortcut for this keystroke.
	void registerHotKey() const;
	
	// Cancels an invocation of registerHotKey().
	// Returns true if a hotkey was actually unregistered, false on noop.
	bool unregisterHotKey() const;
	
	
	// Returns true if this keystroke is a special case (a subset) of the given keystroke.
	// Allows telling if two keystrokes conflict.
	bool isSubset(const Keystroke& other) const;
	
	// Returns whether the system can release the special keys of this keystroke.
	// Media keys do not support special keys releasing.
	bool canReleaseSpecialKeys() const {
		return (VK_BROWSER_BACK > m_vk) ||  // first media key
		       (m_vk > VK_LAUNCH_APP2);     // last media key
	}
	
	// Returns the canonical code of a virtual key code.
	static BYTE canonicalizeKey(BYTE vk) {
		return (vk == VK_CLEAR) ? VK_NUMPAD5 : vk;
	}
	
	// Returns whether a virtual key code is extended
	// according to the lParam argument of GetKeyNameText().
	static bool isKeyExtended(UINT vk);
	
	// Returns the window that has the input focus.
	static HWND getInputFocus();
	
	// Attaches the input focus to the current foreground window.
	// Retrieves the new input window and thread.
	static void catchKeyboardFocus(HWND* new_input_window, DWORD* new_input_thread);
	
	// Detaches the input focus from the given thread.
	static void detachKeyboardFocus(DWORD input_thread);
	
	// Releases the special keys up in the given keyboard state.
	// keyboard_state: see GetKeyboardState().
	static void releaseSpecialKeys(BYTE keyboard_state[]);
	
	// Compares two keystrokes.
	//
	// Returns:
	//   A < 0 integer if keystroke1 is before keystroke2, > 0 if keystroke1 is after keystroke2,
	//   0 if the keystrokes are equal.
	static int compare(const Keystroke& keystroke1, const Keystroke& keystroke2);
	
	// Returns an integer between 0 and 3 inclusive to be used for comparing the mod codes of two
	// keystrokes for a given special key. Mod codes order: none < unsided < left < right.
	//
	// Args:
	//   mod_code: Unsided MOD_* constant identifying the special key.
	//
	// Returns:
	//   - 0 if the keystroke does not use the special key
	//       and has none of the mod codes in remaining_mod_codes_mask.
	//   - 1 if the keystroke is unsided and uses the given special key.
	//   - 2 if the keystroke is sided and uses the given special key as left.
	//   - 3 if the keystroke is sided and uses the given special key as right.
	//   - 0 if the keystroke does not use the special key
	//       and has some of the mod codes in remaining_mod_codes_mask.
	int getModCodeSortKey(int mod_code, DWORD remaining_mod_codes_mask) const;
	
	// Asks the user to supply a keystroke. Blocks.
	// Shows this keystroke as initial value in the dialog box.
	// Stores the result in this instance.
	// Returns true on success, false on cancellation.
	//
	// hwnd_parent: window to use as parent for the dialog box.
	bool showEditDialog(HWND hwnd_parent);
	
	// Asks the user for a keystroke to simulate.
	// Stores the result in this instance.
	// Returns true on success, false on cancellation.
	//
	// hwnd_parent: window to use as parent for the dialog box.
	bool showSendKeysDialog(HWND hwnd_parent);
	
private:
	
	// Releases a key if it is down in the given keyboard state.
	// keyboard_state: the state to read & update; see GetKeyboardState().
	static void releaseKey(BYTE vk, BYTE keyboard_state[]);
};


struct SpecialKey {
	BYTE vk;  // Virtual key code, undistinguishable from left and right if possible
	BYTE vk_left;  // Virtual code of the left key
	BYTE vk_right;  // Virtual code of the right key
	DWORD mod_code;  // MOD_* code of the key (e.g. MOD_CONTROL), not sided
	int tok;  // Token index of the name of the key
};

constexpr SpecialKey e_special_keys[4] = {
	{ VK_LWIN, VK_LWIN, VK_RWIN, MOD_WIN, tokWin },
	{ VK_CONTROL, VK_LCONTROL, VK_RCONTROL, MOD_CONTROL, tokCtrl },
	{ VK_SHIFT, VK_LSHIFT, VK_RSHIFT, MOD_SHIFT, tokShift },
	{ VK_MENU, VK_LMENU, VK_RMENU, MOD_ALT, tokAlt },
};
