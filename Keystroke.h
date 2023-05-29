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

inline constexpr BYTE kKeyDownMask = 0x80;
inline constexpr short kKeyToggledMask = 0x01;

inline constexpr int kRightModCodeOffset = 16;
inline constexpr DWORD kModCodeAll = ~0U;


struct SpecialKey {
	BYTE vk;  // Virtual key code, undistinguishable from left and right if possible
	BYTE vk_left;  // Virtual code of the left key
	BYTE vk_right;  // Virtual code of the right key
	DWORD mod_code;  // MOD_* code of the key (e.g. MOD_CONTROL), not sided
	Token tok;  // Token index of the name of the key

	DWORD left_mod_code() const { return mod_code; }
	DWORD right_mod_code() const { return mod_code << kRightModCodeOffset; }
	DWORD all_mod_codes() const { return left_mod_code() | right_mod_code(); }
};

inline constexpr SpecialKey kSpecialKeys[] = {
	{
		.vk = VK_LWIN,
		.vk_left = VK_LWIN,
		.vk_right = VK_RWIN,
		.mod_code = MOD_WIN,
		.tok = Token::kWin,
	},
	{
		.vk = VK_CONTROL,
		.vk_left = VK_LCONTROL,
		.vk_right = VK_RCONTROL,
		.mod_code = MOD_CONTROL,
		.tok = Token::kCtrl,
	},
	{
		.vk = VK_SHIFT,
		.vk_left = VK_LSHIFT,
		.vk_right = VK_RSHIFT,
		.mod_code = MOD_SHIFT,
		.tok = Token::kShift,
	},
	{
		.vk = VK_MENU,
		.vk_left = VK_LMENU,
		.vk_right = VK_RMENU,
		.mod_code = MOD_ALT,
		.tok = Token::kAlt,
	},
};


class Keystroke {
public:
	
	enum CondType {
		kCondTypeCapsLock,
		kCondTypeNumLock,
		kCondTypeScrollLock,
		kCondTypeCount
	};
	static constexpr int kCondTypeVks[kCondTypeCount] = {
		VK_CAPITAL,
		VK_NUMLOCK,
		VK_SCROLL,
	};
	
	enum class Condition {
		kIgnore,
		kYes,
		kNo,
	};
	static constexpr TCHAR kConditionChars[] = {
		_T('\0'),  // kIgnore
		_T('+'),   // kYes
		_T('-'),   // kNo
	};
	static constexpr int kConditionCount = arrayLength(kConditionChars);
	
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
	
	// Indexed by CondType.
	Condition m_conditions[kCondTypeCount];
	
	// True if m_sided_mod_code distinguishes between left and right special keys.
	bool m_sided;
	
public:
	
	Keystroke() {
		clear();
	}
	
	Keystroke(const Keystroke& other) = default;
	Keystroke& operator =(const Keystroke& other) = default;
	
#ifdef _DEBUG
	String debugString() const {
		String display_name;
		getDisplayName(display_name.getBuffer(kHotKeyBufSize));
		return StringPrintf(_T("<%s> %s"), display_name.getSafe(), conditionsDebugString().getSafe());
	}
	
	String rawDebugString() const {
		return StringPrintf(
			_T("vk=0x%02X (%d) sided=%d sided_mod_code=%d conditions=%s"),
			m_vk, m_vk, m_sided, m_sided_mod_code, conditionsDebugString().getSafe());
	}
	
	String conditionsDebugString() const {
		static constexpr LPCTSTR kDebugConditionChars = _T("/+-");
		return StringPrintf(
			_T("C%cN%cS%c"),
			kDebugConditionChars[int(m_conditions[kCondTypeCapsLock])],
			kDebugConditionChars[int(m_conditions[kCondTypeNumLock])],
			kDebugConditionChars[int(m_conditions[kCondTypeScrollLock])]);
	}
#endif // _DEBUG
	
	// Completely resets this keystroke to the empty state.
	void clear() {
		clearKeys();
		m_sided = false;
		for (auto& condition : m_conditions) {
			condition = Condition::kIgnore;
		}
	}
	
	// Resets the keys of this keystroke, but not the conditions or sided-ness.
	void clearKeys() {
		m_vk = 0;
		m_sided_mod_code = 0;
	}
	
	
	WORD getUnsidedModCode() const {
		return WORD(m_sided_mod_code) | WORD(m_sided_mod_code >> kRightModCodeOffset);
	}
	
	// Initializes the global data that the display name functions need.
	static void loadVkKeyNames();
	
	// Gets the human readable name of this keystroke.
	// Assumes loadVkKeyNames() has been called.
	void getDisplayName(LPTSTR output) const;
	
	// Parses the given human readable name. Accepts the output of getDisplayName().
	// May modify the input. Assumes the keystroke is initially cleared.
	void parseDisplayName(LPTSTR input);
	
	// Returns the display of the given virtual key code, empty if unavailable.
	// Assumes loadVkKeyNames() has been called.
	static const String& getKeyName(UINT vk) {
		return s_vk_key_names[(0 <= vk && vk <= 0xFF) ? vk : 0];
	}
	
	
	// Simulate typing the keystroke: presses the keys down and up.
	// Simulates special keys as unsided.
	// already_down_mod_code: special keys to assume are already down and to keep down, unsided.
	void simulateTyping(DWORD already_down_mod_code) const;
	
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
	
	// Same as detachKeyboardFocus() then catchKeyboardFocus().
	// Overwrites input_thread with the new input thread.
	static void resetKeyboardFocus(HWND* new_input_window, DWORD* input_thread);
	
	// Releases the special keys up in the given keyboard state.
	// keyboard_state: see GetKeyboardState().
	// keep_down_mod_code: the special keys to keep down, unsided.
	static void releaseSpecialKeys(BYTE keyboard_state[], DWORD keep_down_mod_code);
	
	// Compares two keystrokes.
	//
	// Returns:
	//   A < 0 integer if keystroke1 is before keystroke2, > 0 if keystroke1 is after keystroke2,
	//   0 if the keystrokes are equal.
	static int compare(const Keystroke& keystroke1, const Keystroke& keystroke2);
	
	// Returns a >= 0 integer to be used for comparing the mod codes of two keystrokes
	// for a given special key. Mod codes order: none < unsided < left < right < left + right.
	//
	// Args:
	//   special_key: the compared special key
	//   remaining_mod_codes_mask: the mod codes mask of the special keys that follow special_key.
	int getModCodeSortKey(const SpecialKey& special_key, DWORD remaining_mod_codes_mask) const;
	
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
	
	// Retrieves the name of a virtual key.
	static void loadVkKeyName(BYTE vk, String* output);
	
	// Name of each virtual key, empty if unknown.
	static String s_vk_key_names[256];
	
	// s_next_named_vk[vk] is the lowest vk_next > vk
	// such that s_next_named_vk[vk_next] is not empty.
	// s_next_named_vk[vk] = 0xFF if no such vk_next exists.
	static BYTE s_next_named_vk[256];
};
