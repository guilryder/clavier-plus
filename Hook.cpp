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
#include "Hook.h"
#include "Shortcut.h"

namespace keyboard_hook {

const DWORD uninstall_timeout = 5000;  // milliseconds

static HHOOK s_keyboard_hook;

static HWND s_hwnd_catch_all_keys;

// Specifies which special keys are currently down. Meaningful only if s_hwnd_catch_all_keys is not
// NULL. This is a bitmask:
//   special_key_index * 2: "left key is down" bit
//   special_key_index * 2 + 1: "right key is down" bit
static DWORD s_special_keys_down_mask;

// Specifies the special keys we are waiting for being down.
static DWORD s_special_keys_waiting_for_down_mask;

// Low-level keyboard hook procedure (WH_KEYBOARD_LL hook). Calls redirectHookMessage() or
// processShortcutHookMessage() if the key message should be processed.
static LRESULT CALLBACK keyboardHookCallback(int code, WPARAM wParam, LPARAM lParam);

// Redirects a keyboard hook message to s_hwnd_catch_all_keys. Updates s_special_keys_down_mask if
// the caught key is a special key.
static void redirectHookMessage(UINT message, const KBDLLHOOKSTRUCT& data);

// Processes a keyboard hook message for shortcuts.
//
// Returns:
//   true if the message has been processed and should be removed from the message queue,
//   false if the message should be forwarded to the next hook in the chain.
static bool processShortcutHookMessage(UINT message, const KBDLLHOOKSTRUCT& data);

static HANDLE s_shortcut_executing_thread_event;  // Set to wake shortcutExecutingThread().
static HANDLE s_shortcut_executing_thread;  // The only shortcutExecutingThread() thread.
static bool s_executing;  // Set by shortcutExecutingThread() while executing a shortcut.
static bool s_want_quit;  // Set when shortcutExecutingThread() should quit.

// Background thread that executes the matching shortcuts. Uses the matching result stored in
// shortcut::e_matching_level. It is an infinite loop waiting for s_shortcut_executing_thread_event
// to be set. It stops when when s_want_quit is true.
//
// Returns:
//   true if the message has been processed and should be removed from the message queue,
//   false if the message should be forwarded to the next hook in the chain.
static DWORD WINAPI shortcutExecutingThread(void* param);

// Executes one of the shortcuts according to their matching level and the results stored in
// shortcut::e_matching_level.
static void executeMatchingShortcut();

void install() {
	s_hwnd_catch_all_keys = NULL;
	s_executing = false;
	s_want_quit = false;
	s_shortcut_executing_thread_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	s_keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHookCallback, e_hInst, 0);
	DWORD thread_id;
	s_shortcut_executing_thread = CreateThread(NULL, 0, shortcutExecutingThread, NULL, 0, &thread_id);
}

void uninstall() {
	// Close the shortcut executing thread.
	s_want_quit = true;
	SetEvent(s_shortcut_executing_thread_event);
	WaitForSingleObject(s_shortcut_executing_thread, uninstall_timeout);
	TerminateThread(s_shortcut_executing_thread_event, 0);
	CloseHandle(s_shortcut_executing_thread);
	
	UnhookWindowsHookEx(s_keyboard_hook);
}

void setCatchAllKeysWindow(HWND hwnd) {
	s_hwnd_catch_all_keys = hwnd;
	s_special_keys_down_mask = 0;
}

LRESULT CALLBACK keyboardHookCallback(int code, WPARAM wParam, LPARAM lParam) {
	if (code >= 0) {
		const KBDLLHOOKSTRUCT& data = *reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
		if (s_hwnd_catch_all_keys) {
			redirectHookMessage(static_cast<UINT>(wParam), data);
			return TRUE;
		} else if (!e_hdlgModal && processShortcutHookMessage(static_cast<UINT>(wParam), data)) {
			return TRUE;
		}
	}
	// Default processing.
	return CallNextHookEx(s_keyboard_hook, code, wParam, lParam);
}

void redirectHookMessage(UINT message, const KBDLLHOOKSTRUCT& data) {
	const DWORD vk_caught = data.vkCode;
	bool is_down;
	switch (message) {
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			is_down = true;
			break;
		
		case WM_KEYUP:
		case WM_SYSKEYUP:
			is_down = false;
			break;
		
		default:
			// Ignore all other keyboard messages.
			return;
	}
	
	// Updates s_special_keys_down_mask if the current key is a special key.
	for (int special_key = 0; special_key < arrayLength(e_aSpecialKey); special_key++) {
		const BYTE vk_left = e_aSpecialKey[special_key].vkLeft;
		const BYTE vk_right = (BYTE)(vk_left + 1);
		
		DWORD special_key_mask;
		if (vk_caught == vk_left) {
			special_key_mask = 1L << (special_key * 2);
		} else if (vk_caught == vk_right) {
			special_key_mask = 1L << (special_key * 2 + 1);
		} else {
			continue;
		}
		
		if (is_down) {
			s_special_keys_down_mask |= special_key_mask;
		} else {
			s_special_keys_down_mask &= ~special_key_mask;
		}
		break;  // Optimization: vk_caught can match only one special key
	}
	
	PostMessage(
		s_hwnd_catch_all_keys,
		(is_down) ? WM_CLAVIER_KEYDOWN : WM_CLAVIER_KEYUP,
		vk_caught,
		s_special_keys_down_mask);
}

bool processShortcutHookMessage(UINT message, const KBDLLHOOKSTRUCT& data) {
	VERIF(message == WM_KEYDOWN || message == WM_SYSKEYDOWN);
	
	// Do not process simulated keystrokes.
	VERIF(!(data.flags & LLKHF_INJECTED));
	
#ifdef _DEBUG
	TCHAR log_line[1024];
	wsprintf(log_line, _T("message: %08lX - vk: %3lu scan: %3lu flags: %8X\n"),
		message, data.vkCode, data.scanCode, data.flags);
	OutputDebugString(log_line);
#endif  // _DEBUG
	
	Keystroke ks;
	ks.m_vk = Keystroke::filterVK((BYTE)data.vkCode);
	ks.m_vkFlags = 0;
	ks.m_bDistinguishLeftRight = true;
	
	// Test for right special keys
	for (int special_key = 0; special_key < arrayLength(e_aSpecialKey); special_key++) {
		const DWORD vk_flags = e_aSpecialKey[special_key].vkFlags;
		const BYTE vk_left = e_aSpecialKey[special_key].vkLeft;
		const BYTE vk_right = (BYTE)(vk_left + 1);
		if (isKeyDown(vk_right)) {
			// Test for right key first, since left special key codes are the same as "unsided" key codes.
#ifdef _DEBUG
			wsprintf(log_line, _T("  Right key down: %lu\n"), vk_left);
			OutputDebugString(log_line);
#endif  // _DEBUG
			ks.m_vkFlags |= vk_flags << vkFlagsRightOffset;
		} else if (isKeyDown(vk_left)) {
#ifdef _DEBUG
			wsprintf(log_line, _T("  Left key down: %lu\n"), vk_left);
			OutputDebugString(log_line);
#endif  // _DEBUG
			ks.m_vkFlags |= vk_flags;
		}
	}
	
	const HWND hwnd_focus = Keystroke::getKeyboardFocus();
	
	// Retrieve the state of each condition key. Condition keys are all toggle keys.
	static const int vk_by_cond_type[] = {
		VK_CAPITAL, VK_NUMLOCK, VK_SCROLL,
	};
	for (int cond_type = 0; cond_type < condTypeCount; cond_type++) {
		ks.m_aCond[cond_type] = (GetKeyState(vk_by_cond_type[cond_type]) & 0x01) ? condYes : condNo;
	}
	
	// Get the current window process name and title, for conditions checking
	TCHAR process_name[MAX_PATH];
	if (!getWindowProcessName(hwnd_focus, process_name)) {
		*process_name = _T('\0');
	}
	TCHAR window_title[bufWindowTitle];
	if (!GetWindowText(hwnd_focus, window_title, arrayLength(window_title))) {
		*window_title = _T('\0');
	}
	
	shortcut::computeAllMatchingLevels(ks, process_name, window_title);
	if (shortcut::e_matching_result.matching_shortcuts_count == 0) {
		// No matching shortcut: perform default action for the keystroke.
		return false;
	}
	
	if (!s_executing) {
		s_executing = true;
		SetEvent(s_shortcut_executing_thread_event);
	}
	
	return true;
}

DWORD WINAPI shortcutExecutingThread(void* MY_UNUSED(param)) {
	for (;;) {
		WaitForSingleObject(s_shortcut_executing_thread_event, INFINITE);
		if (s_want_quit) {
			break;
		}
		
		shortcut::GuardList guard;
		executeMatchingShortcut();
		s_executing = false;
	}
	return 0;
}

void executeMatchingShortcut() {
	using shortcut::e_matching_result;
	using shortcut::getNextMatching;
	
	if (e_matching_result.matching_shortcuts_count == 1) {
		// Exactly one matching shortcut: execute it right now.
		getNextMatching(shortcut::getFirst(), e_matching_result.max_matching_level)->execute(true);
		
	} else {
		// Several matching shortcuts: let the user choose which one he wants to execute.
		for (Shortcut* matching_shortcut = shortcut::getFirst();;
				matching_shortcut = matching_shortcut->getNext()) {
			matching_shortcut = getNextMatching(matching_shortcut, e_matching_result.max_matching_level);
			if (!matching_shortcut) {
				break;
			}
			///$$$ TODO
		}
	}
}

}  // keyboard_hook namespace
