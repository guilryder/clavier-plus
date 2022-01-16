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
#include "I18n.h"
#include "Shortcut.h"

#include <algorithm>

namespace shortcut {

static Shortcut* s_first_shortcut;

static constexpr int s_default_column_widths[] = { 35, 20, 15, 10 };

constexpr WCHAR kUtf16LittleEndianBom = 0xFEFF;


enum class LastTextExecution {
	None,
	Text,
	Keystroke,
};

struct ExecutionContext {
	BYTE keyboard_state[256];
	
	// Unsided MOD_* constants bitmask.
	DWORD keep_down_mod_code;
	
	HWND input_window;
	DWORD input_thread;
	
	LastTextExecution lastTextExecution;
};


// []
// Sleep for 100 milliseconds and catch the focus.
// Convenience alias for [{Focus,100}].
// Reads & updates input_thread and input_window in the context.
static void commandEmpty(ExecutionContext* context);

// [{Wait,duration}]
// Sleep for a given number of milliseconds.
static void commandWait(LPTSTR arg);

// [{Focus,delay,[!]window_name}]
// Sleep for delay milliseconds and catch the focus.
// If window_name does not begin with '!', return false if the window is not found.
// Reads & updates input_thread and input_window in the context.
static bool commandFocus(ExecutionContext* context, LPTSTR arg);

// [{FocusOrLaunch,window_name,command,delay}]
// Activate window_name.
// If the window is not found, relases any pressed special keys, execute command, then sleep for delay milliseconds.
// Either way, catch the focus (reads & updates input_thread and input_window in the context).
static void commandFocusOrLaunch(ExecutionContext* context, LPTSTR arg);

// [{Copy,text}]
// Copy the text argument to the clipboard.
static void commandCopy(LPTSTR arg);

// [{Mouse,state}] where state is 2 letters:
// 1 letter for button: L (left), M (middle), R (right)
// 1 letter for state: U (up), D (down)
// Simulate mouse clicks.
static void commandMouseButton(LPTSTR arg);

// [{MouseMoveTo,x,y}], [{MouseMoveToFocus,x,y}], [{MouseMoveBy,dx,dy}]
// Move the mouse cursor.
// arg: move coordinates relative to origin_point.
static void commandMouseMove(POINT origin_point, LPTSTR arg);

// [{MouseWheel,offset}]
// Simulate a mouse wheel scroll.
static void commandMouseWheel(LPTSTR arg);

// [{KeysDown,keystroke}]
// Keep special keys down.
// Reads & updates keep_down_unsided_mod_code.
static void commandKeysDown(ExecutionContext* context, LPTSTR arg);


// Return whether to continue executing the shortcut.
static bool executeSpecialCommand(LPCTSTR shortcut_start, LPCTSTR& shortcut_end, ExecutionContext* context);

static void executeCommandLine(LPCTSTR command, ExecutionContext* context);

static void simulateCharacter(TCHAR c, DWORD keep_down_mod_code);

static void focusWindow(HWND hwnd);


constexpr LPCTSTR kLineSeparator = _T("-\r\n");


void initialize() {
	s_first_shortcut = nullptr;
}

void terminate() {
}

Shortcut* getFirst() {
	return s_first_shortcut;
}

Shortcut* find(const Keystroke& ks, LPCTSTR program) {
	Shortcut *best_shortcut = nullptr;
	for (Shortcut* shortcut = getFirst(); shortcut; shortcut = shortcut->getNext()) {
		if (shortcut->isSubset(ks, program)) {
			if (!best_shortcut || !best_shortcut->m_programs_only) {
				best_shortcut = shortcut;
			}
		}
	}
	return best_shortcut;
}


Shortcut::Shortcut(const Shortcut& sh) : Keystroke(sh) {
	m_next_shortcut = nullptr;
	m_type = sh.m_type;
	m_show_option = sh.m_show_option;
	m_support_file_open = sh.m_support_file_open;
	m_programs_only = sh.m_programs_only;
	m_small_icon_index = sh.m_small_icon_index;
	m_icon = CopyIcon(sh.m_icon);
	
	m_description = sh.m_description;
	m_text = sh.m_text;
	m_command = sh.m_command;
	m_directory = sh.m_directory;
	m_programs = sh.m_programs;
	
	m_usage_count = sh.m_usage_count;
}

Shortcut::Shortcut(const Keystroke& ks) : Keystroke(ks) {
	m_next_shortcut = nullptr;
	m_type = Shortcut::Type::Text;
	m_show_option = SW_NORMAL;
	m_support_file_open = false;
	m_programs_only = false;
	m_small_icon_index = iconNeeded;
	m_icon = NULL;
	
	m_usage_count = 0;
}

void Shortcut::addToList() {
	m_next_shortcut = s_first_shortcut;
	s_first_shortcut = this;
}


void Shortcut::save(HANDLE file) {
	cleanPrograms();
	
	TCHAR display_name[bufHotKey], code_text[bufCode];
	getDisplayName(display_name);
	wsprintf(code_text, _T("%lu"), static_cast<DWORD>(m_vk) | (m_sided_mod_code << 8));
	
	struct LINE {
		int tokKey;
		LPCTSTR pszValue;
	};
	LINE lines[3 + 4 + 2 + condTypeCount + 1];
	lines[0].tokKey = tokShortcut;
	lines[0].pszValue = display_name;
	lines[1].tokKey = tokCode;
	lines[1].pszValue = code_text;
	int line_count = 2;
	
	if (m_sided) {
		lines[line_count].tokKey = tokDistinguishLeftRight;
		lines[line_count].pszValue = _T("1");
		line_count++;
	}
	
	for (int i = 0; i < condTypeCount; i++) {
		if (m_conditions[i] != condIgnore) {
			lines[line_count].tokKey = tokConditionCapsLock + i;
			lines[line_count].pszValue = getToken(m_conditions[i] - 1 + tokConditionYes);
			line_count++;
		}
	}
	
	String text;
	
	switch (m_type) {
	case Shortcut::Type::Command:
		lines[line_count].tokKey = tokCommand;
		lines[line_count].pszValue = m_command;
		line_count++;
		
		if (m_directory.isSome()) {
			lines[line_count].tokKey = tokDirectory;
			lines[line_count].pszValue = m_directory;
			line_count++;
		}
		
		lines[line_count].tokKey = tokWindow;
		lines[line_count].pszValue = _T("");
		for (int i = 0; i < arrayLength(shortcut::show_options); i++) {
			if (m_show_option == shortcut::show_options[i]) {
				lines[line_count].pszValue = getToken(tokShowNormal + i);
				break;
			}
		}
		line_count++;
		
		if (m_support_file_open) {
			lines[line_count].tokKey = tokSupportFileOpen;
			lines[line_count].pszValue = _T("1");
			line_count++;
		}
		break;
		
	case Shortcut::Type::Text:
		// Copy the text, replace "\r\n" by "\r\n>"
		// to handle multiple lines text
		for (const TCHAR *pcFrom = m_text; *pcFrom; pcFrom++) {
			text += *pcFrom;
			if (pcFrom[0] == _T('\r') && pcFrom[1] == _T('\n')) {
				text += _T("\n>");
				pcFrom++;
			}
		}
		
		lines[line_count].tokKey = tokText;
		lines[line_count].pszValue = text;
		line_count++;
		break;
	}
	
	if (m_programs.isSome()) {
		lines[line_count].tokKey = (m_programs_only) ? tokPrograms : tokAllProgramsBut;
		lines[line_count].pszValue = m_programs;
		line_count++;
	}
	
	if (m_description.isSome()) {
		lines[line_count].tokKey = tokDescription;
		lines[line_count].pszValue = m_description;
		line_count++;
	}
	
	TCHAR strbuf_usage_count[i18n::bufIntegerCount];
	wsprintf(strbuf_usage_count, _T("%d"), m_usage_count);
	lines[line_count].tokKey = tokUsageCount;
	lines[line_count].pszValue = strbuf_usage_count;
	line_count++;
	
	// Write all
	for (int i = 0; i < line_count; i++) {
		writeFile(file, getToken(lines[i].tokKey));
		writeFile(file, _T("="));
		writeFile(file, lines[i].pszValue);
		writeFile(file, _T("\r\n"));
	}
	writeFile(file, kLineSeparator);
}

bool Shortcut::load(LPTSTR* input) {
	// Read
	
	int tokKey = tokNotFound;
	for (;;) {
		
		// Read the line
		LPTSTR line_start = *input;
		if (!line_start) {
			break;
		}
		LPTSTR line_end = line_start;
		while (*line_end && *line_end != _T('\n') && *line_end != _T('\r')) {
			line_end++;
		}
		if (*line_end) {
			*line_end = _T('\0');
			*input = line_end + 1;
		} else {
			*input = nullptr;
		}
		
		INT_PTR line_length = line_end - line_start;
		
		// If end of shortcut, stop
		if (*line_start == kLineSeparator[0]) {
			break;
		}
		
		// If the line is empty, ignore it
		if (!line_length) {
			continue;
		}
		
		// If next line of text, get it
		if (*line_start == _T('>') && tokKey == tokText) {
			m_text += _T("\r\n");
			m_text += line_start + 1;
			continue;
		}
		
		// Get the key name
		TCHAR *next_sep = line_start;
		while (*next_sep && *next_sep != _T(' ') && *next_sep != _T('=')) {
			next_sep++;
		}
		const TCHAR next_sep_copy = *next_sep;
		*next_sep = _T('\0');
		
		// Identify the key
		tokKey = findToken(line_start);
		if (tokKey == tokNotFound) {
			continue;
		}
		
		// Get the value
		*next_sep = next_sep_copy;
		while (*next_sep && *next_sep != _T('=')) {
			next_sep++;
		}
		if (*next_sep) {
			next_sep++;
		}
		line_length -= next_sep - line_start;
		switch (tokKey) {
			
			// Language
			case tokLanguage:
				for (int lang = 0; lang < i18n::langCount; lang++) {
					if (!lstrcmpi(next_sep, getLanguageName(lang))) {
						i18n::setLanguage(lang);
					}
				}
				break;
			
			// Main window size
			case tokSize:
				while (*next_sep == _T(' ')) {
					next_sep++;
				}
				e_sizeMainDialog.cx = StrToInt(parseCommaSepArg(next_sep));
				e_sizeMainDialog.cy = StrToInt(parseCommaSepArg(next_sep));
				e_maximize_main_dialog = toBool(StrToInt(parseCommaSepArg(next_sep)));
				e_icon_visible = !StrToInt(parseCommaSepArg(next_sep));
				break;
			
			// Main window columns width
			case tokColumns:
				for (int i = 0; i < colCount - 1; i++) {
					if (!*next_sep) {
						break;
					}
					int cx = StrToInt(parseCommaSepArg(next_sep));
					if (cx >= 0) {
						e_column_widths[i] = cx;
					}
				}
				break;
			
			// Sorting column
			case tokSorting:
				s_sort_column = StrToInt(next_sep);
				if (s_sort_column < 0 || s_sort_column >= colCount) {
					s_sort_column = colContents;
				}
				break;
			
			// Shortcut
			case tokShortcut:
				Keystroke::parseDisplayName(next_sep);
				break;
			
			// Code
			case tokCode:
				if (!m_vk) {
					const DWORD dwCode = static_cast<DWORD>(StrToInt(next_sep));
					if (dwCode) {
						m_vk = canonicalizeKey(LOBYTE(dwCode));
						m_sided_mod_code = dwCode >> 8;
					}
				}
				break;
			
			// Distinguish left/right
			case tokDistinguishLeftRight:
				m_sided = toBool(StrToInt(next_sep));
				break;
			
			// Description
			case tokDescription:
				m_description = next_sep;
				break;
			
			// Text
			case tokText:
				m_type = Shortcut::Type::Text;
				m_text = next_sep;
				break;

			// Command
			case tokCommand:
				m_type = Shortcut::Type::Command;
				m_command = next_sep;
				break;
			
			// Directory
			case tokDirectory:
				m_directory = next_sep;
				break;
			
			// Programs
			case tokPrograms:
			case tokAllProgramsBut:
				m_programs = next_sep;
				m_programs_only = (tokKey == tokPrograms);
				cleanPrograms();
				break;
			
			// Window
			case tokWindow:
				{
					const int show_option_index = findToken(next_sep) - tokShowNormal;
					if (0 <= show_option_index && show_option_index < arrayLength(shortcut::show_options)) {
						m_show_option = shortcut::show_options[show_option_index];
					}
				}
				break;
			
			// Folder
			case tokSupportFileOpen:
				m_support_file_open = toBool(StrToInt(next_sep));
				break;
			
			// Condition
			case tokConditionCapsLock:
			case tokConditionNumLock:
			case tokConditionScrollLock:
				{
					int cond = findToken(next_sep);
					if (tokConditionYes <= cond && cond <= tokConditionNo) {
						m_conditions[tokKey - tokConditionCapsLock] =
							static_cast<KeystrokeCondition>(cond - tokConditionYes + condYes);
					}
				}
				break;
			
			// Usage count
			case tokUsageCount:
				m_usage_count = std::max(0, StrToInt(next_sep));
				break;
		}
	}
	
	// Valid shortcut
	VERIF(m_vk);
	
	// No conflict between shortcuts, except concerning programs:
	// there can be one programs-conditions-less shortcut
	// and any count of shortcuts having different programs conditions
	String *const programs = getPrograms();
	bool ok = true;
	for (Shortcut* shortcut = getFirst(); shortcut; shortcut = shortcut->getNext()) {
		if (shortcut->testConflict(*this, programs, m_programs_only)) {
			ok = false;
			break;
		}
	}
	delete [] programs;
	
	return ok;
}


void Shortcut::execute(bool from_hotkey) {
	if (from_hotkey) {
		m_usage_count++;
	}
	
	ExecutionContext context;
	GetKeyboardState(context.keyboard_state);
	context.keep_down_mod_code = 0;
	
	// Typing simulation requires the application has the keyboard focus
	Keystroke::catchKeyboardFocus(&context.input_window, &context.input_thread);
	
	const bool can_release_special_keys = from_hotkey && canReleaseSpecialKeys();
	
	if (can_release_special_keys) {
		BYTE new_keyboard_state[256];
		memcpy(new_keyboard_state, context.keyboard_state, sizeof(context.keyboard_state));
		
		// Simulate special keys release via SetKeyboardState, to avoid side effects.
		// Avoid to leave Alt and Shift down, alone, at the same time
		// because it is one of Windows keyboard layout shortcut:
		// simulate Ctrl down, release Alt and Shift, then release Ctrl
		bool release_control = false;
		if ((context.keyboard_state[VK_MENU] & keyDownMask) &&
				(context.keyboard_state[VK_SHIFT] & keyDownMask) &&
				!(context.keyboard_state[VK_CONTROL] & keyDownMask)) {
			release_control = true;
			keybdEvent(VK_CONTROL, /* down= */ true);
			new_keyboard_state[VK_CONTROL] = keyDownMask;
		}
		
		for (int i = 0; i < arrayLength(e_special_keys); i++) {
			const SpecialKey& special_key = e_special_keys[i];
			new_keyboard_state[special_key.vk] =
			new_keyboard_state[special_key.vk_left] =
			new_keyboard_state[special_key.vk_right] = 0;
		}
		new_keyboard_state[m_vk] = 0;
		SetKeyboardState(new_keyboard_state);
		
		if (release_control) {
			keybdEvent(VK_CONTROL, /* down= */ false);
		}
	}
	
	switch (m_type) {
	case Shortcut::Type::Command:
		// Required because the launched program
		// can be a script that simulates keystrokes
		if (can_release_special_keys) {
			releaseSpecialKeys(context.keyboard_state, /* keep_down_mod_code= */ 0);
		}
		
		if (!m_support_file_open || !setDialogBoxDirectory(context.input_window, m_command)) {
			clipboardToEnvironment();
			ShellExecuteThread* const shell_execute_thread = new ShellExecuteThread(
				m_command, m_directory, m_show_option);
			startThread(shell_execute_thread->thread, shell_execute_thread);
		}
		break;
		
	case Shortcut::Type::Text:
		// Special keys to keep down across commands.
		
		LastTextExecution lastTextExecution = LastTextExecution::None;
		
		// Send the text to the window
		bool escaping = false;  // whether the next character is '\'-escaped
		LPCTSTR text = m_text;
		for (size_t i = 0; text[i]; i++) {
			const WORD c = static_cast<WORD>(text[i]);
			if (c == _T('\n')) {
				// Skip '\n': redundant with the expected '\r'.
				continue;
			}
			
			if (!escaping && c == _T('\\')) {
				escaping = true;
				continue;
			}
			
			if (escaping || c != _T('[')) {
				// Regular character: send WM_CHAR.
				if (lastTextExecution != LastTextExecution::Text) {
					lastTextExecution = LastTextExecution::Text;
					sleepBackground(0);
				}
				
				escaping = false;
				const WORD vkMask = VkKeyScan(c);
				PostMessage(context.input_window, WM_CHAR, c,
					MAKELPARAM(1, MapVirtualKey(LOBYTE(vkMask), 0)));
				
			} else {
				// '[': inline shortcut, or inline command line execution
				
				// Extract the inside of the shortcut.
				// Take into account '\' escaping to detect the end of the shortcut, but do not unescape.
				const LPCTSTR shortcut_start = text + i + 1;
				const TCHAR *shortcut_end = shortcut_start;
				bool escaping2 = false;
				while (*shortcut_end && !(*shortcut_end == _T(']') && !escaping2)) {
					escaping2 = !escaping2 && (*shortcut_end == _T('\\'));
					shortcut_end++;
				}
				if (!*shortcut_end) {
					// Non-terminated command.
					break;
				}
				
				if (!executeSpecialCommand(shortcut_start, shortcut_end, &context)) {
					break;
				}
				
				i = shortcut_end - text;
			}
		}
		
		// Release all special keys kept down in case the shortcut doesn't end with [{KeysDown}].
		releaseSpecialKeys(context.keyboard_state, /* keep_down_mode_code= */ ~context.keep_down_mod_code);
		break;
	}
}


bool executeSpecialCommand(LPCTSTR shortcut_start, LPCTSTR& shortcut_end, ExecutionContext* context) {
	String inside(shortcut_start, static_cast<int>(shortcut_end - shortcut_start));
	
	if (inside.isEmpty()) {
		// []
		
		context->lastTextExecution = LastTextExecution::None;
		commandEmpty(context);
		return true;
	}
	
	if (*shortcut_start == _T('[') && *shortcut_end == _T(']')) {
		// Double brackets: [[command line]]
		
		shortcut_end++;
		const LPTSTR command_line = &inside[1];
		unescape(command_line);
		
		executeCommandLine(command_line, context);
		
	} else if (*shortcut_start == _T('{') && shortcut_end[-1] == _T('}')) {
		// Braces: [{command}]
		
		inside[inside.getLength() - 1] = _T('\0');
		LPTSTR arg = &inside[1];
		const LPCTSTR command = parseCommaSepArgUnescape(arg);
		
		if (!lstrcmpi(command, _T("Wait"))) {
			commandWait(arg);
		} else if (!lstrcmpi(command, _T("Focus"))) {
			return commandFocus(context, arg);
		} else if (!lstrcmpi(command, _T("FocusOrLaunch"))) {
			commandFocusOrLaunch(context, arg);
		} else if (!lstrcmpi(command, _T("Copy"))) {
			commandCopy(arg);
		} else if (!lstrcmpi(command, _T("MouseButton"))) {
			commandMouseButton(arg);
		} else if (!lstrcmpi(command, _T("MouseMoveTo"))) {
			const POINT origin_point = { 0, 0 };
			commandMouseMove(origin_point, arg);
		} else if (!lstrcmpi(command, _T("MouseMoveToFocus"))) {
			RECT focused_window_rect;
			const HWND hwnd_owner = GetAncestor(context->input_window, GA_ROOT);
			if (!hwnd_owner || !GetWindowRect(hwnd_owner, &focused_window_rect)) {
				focused_window_rect.left = focused_window_rect.top = 0;
			}
			commandMouseMove(reinterpret_cast<const POINT&>(focused_window_rect), arg);
		} else if (!lstrcmpi(command, _T("MouseMoveBy"))) {
			POINT origin_point;
			GetCursorPos(&origin_point);
			commandMouseMove(origin_point, arg);
		} else if (!lstrcmpi(command, _T("MouseWheel"))) {
			commandMouseWheel(arg);
		} else if (!lstrcmpi(command, _T("KeysDown"))) {
			commandKeysDown(context, arg);
		}
	} else {
		// Simple brackets: [keystroke]
		
		Shortcut::releaseSpecialKeys(context->keyboard_state, context->keep_down_mod_code);
		
		context->lastTextExecution = LastTextExecution::Keystroke;
		
		if (*shortcut_start == _T('|') && shortcut_end[-1] == _T('|')) {
			// Brackets and pipe: [|characters as keystroke|]
			
			inside[inside.getLength() - 1] = _T('\0');
			for (LPCTSTR chr_ptr = inside; *++chr_ptr;) {
				if (*chr_ptr != _T('\n')) {  // '\n' is redundant with '\r'.
					simulateCharacter(*chr_ptr, context->keep_down_mod_code);
				}
			}
			
		} else {
			// Simple brackets: [keystroke]
			
			Keystroke keystroke;
			keystroke.parseDisplayName(inside.get());
			keystroke.m_sided_mod_code |= context->keep_down_mod_code;
			keystroke.simulateTyping(/* already_down_mod_code= */ context->keep_down_mod_code);
		}
	}
	
	return true;
}


void executeCommandLine(LPCTSTR command, ExecutionContext* context) {
	// Required because the command can be a script that simulates keystrokes.
	Keystroke::releaseSpecialKeys(context->keyboard_state, context->keep_down_mod_code);
	
	clipboardToEnvironment();
	shellExecuteCmdLine(command, /* directory= */ nullptr, SW_SHOWDEFAULT);
}


void simulateCharacter(TCHAR c, DWORD keep_down_mod_code) {
	Keystroke ks;
	const WORD key = VkKeyScan(c);
	if (key == WORD(-1)) {
		// The character has no keystroke:
		// simulate Alt + Numpad 0 + Numpad ASCII code digits.
		
		// Press Alt.
		ks.m_vk = VK_MENU;
		ks.m_sided_mod_code = 0;
		const bool alt_is_hotkey = ks.unregisterHotKey();
		const BYTE scan_code_alt = static_cast<BYTE>(MapVirtualKey(VK_MENU, MAPVK_VK_TO_VSC));
		keybd_event(VK_MENU, scan_code_alt, 0, 0);
		
		// Press the digits.
		TCHAR digits[5];
		wsprintf(digits, _T("0%u"), static_cast<WORD>(c));
		ks.m_sided_mod_code = MOD_ALT;
		for (size_t digit_index = 0; digits[digit_index]; digit_index++) {
			ks.m_vk = VK_NUMPAD0 + static_cast<BYTE>(digits[digit_index] - _T('0'));
			ks.simulateTyping(/* already_down_mod_code= */ keep_down_mod_code | MOD_ALT);
		}
		
		// Release Alt.
		if (!(keep_down_mod_code & MOD_ALT)) {
			keybd_event(VK_MENU, scan_code_alt, KEYEVENTF_KEYUP, /* dwExtraInfo= */ 0);
		}
		if (alt_is_hotkey) {
			ks.registerHotKey();
		}
		
	} else {
		// The character has a keystroke: simulate it.
		
		const BYTE flags = HIBYTE(key);
		ks.m_vk = LOBYTE(key);
		ks.m_sided_mod_code = keep_down_mod_code;
		if (flags & (1 << 0)) {
			ks.m_sided_mod_code |= MOD_SHIFT;
		}
		if (flags & (1 << 1)) {
			ks.m_sided_mod_code |= MOD_CONTROL;
		}
		if (flags & (1 << 2)) {
			ks.m_sided_mod_code |= MOD_ALT;
		}
		
		ks.simulateTyping(/* already_down_mod_code= */ keep_down_mod_code);
	}
}


void focusWindow(HWND hwnd) {
	// Restore the window if it is minimized.
	WINDOWPLACEMENT wp;
	if (GetWindowPlacement(hwnd, &wp) && (wp.showCmd == SW_SHOWMINIMIZED)) {
		ShowWindow(hwnd, SW_RESTORE);
	}
	
	SetForegroundWindow(hwnd);
}


void commandEmpty(ExecutionContext* context) {
	sleepBackground(100);
	Keystroke::resetKeyboardFocus(&context->input_window, &context->input_thread);
}


void commandWait(LPTSTR arg) {
	sleepBackground(StrToInt(arg));
}


bool commandFocus(ExecutionContext* context, LPTSTR arg) {
	// Parse and apply the delay.
	const int delay_ms = StrToInt(parseCommaSepArgUnescape(arg));
	sleepBackground(delay_ms);
	
	// Unescape the window name argument. Detect the '!' prefix.
	const bool ignore_not_found = (arg[0] == _T('!'));
	if (ignore_not_found) {
		arg++;
	}
	const LPCTSTR window_name = parseCommaSepArgUnescape(arg);
	
	if (*window_name) {
		const HWND hwnd_target = findWindowByName(window_name);
		if (hwnd_target) {
			// Window found: give it the focus.
			focusWindow(hwnd_target);
		} else if (!ignore_not_found) {
			return false;
		}
	}
	
	Keystroke::resetKeyboardFocus(&context->input_window, &context->input_thread);
	return true;
}


void commandFocusOrLaunch(ExecutionContext* context, LPTSTR arg) {
	const LPCTSTR window_name = parseCommaSepArgUnescape(arg);
	const HWND hwnd_target = findWindowByName(window_name);
	if (hwnd_target) {
		// Window found: give it the focus.
		focusWindow(hwnd_target);
	} else {
		// Window not found: execute the command then apply the delay.
		executeCommandLine(parseCommaSepArgUnescape(arg), context);
		const int delay_ms = StrToInt(parseCommaSepArgUnescape(arg));
		sleepBackground(delay_ms);
	}
	
	Keystroke::resetKeyboardFocus(&context->input_window, &context->input_thread);
}


void commandCopy(LPTSTR arg) {
	setClipboardText(arg);
}


void commandMouseButton(LPTSTR arg) {
	CharUpper(arg);
	DWORD dwFlags = 0;
	switch (lstrlen(arg)) {
		case 2:
			switch (MAKELONG(arg[1], arg[0])) {
				case 'L\0D':  dwFlags = MOUSEEVENTF_LEFTDOWN;  break;
				case 'L\0U':  dwFlags = MOUSEEVENTF_LEFTUP;  break;
				case 'M\0D':  dwFlags = MOUSEEVENTF_MIDDLEDOWN;  break;
				case 'M\0U':  dwFlags = MOUSEEVENTF_MIDDLEUP;  break;
				case 'R\0D':  dwFlags = MOUSEEVENTF_RIGHTDOWN;  break;
				case 'R\0U':  dwFlags = MOUSEEVENTF_RIGHTUP;  break;
			}
			if (dwFlags) {
				mouse_event(dwFlags, 0, 0, 0, 0);
				sleepBackground(0);
			}
			break;
		
		case 1:
			switch (*arg) {
				case 'L':  dwFlags = MOUSEEVENTF_LEFTDOWN;  break;
				case 'M':  dwFlags = MOUSEEVENTF_MIDDLEDOWN;  break;
				case 'R':  dwFlags = MOUSEEVENTF_RIGHTDOWN;  break;
			}
			if (dwFlags) {
				mouse_event(dwFlags, 0, 0, 0, 0);
				sleepBackground(0);
				mouse_event(dwFlags * 2, 0, 0, 0, 0);
				sleepBackground(0);
			}
			break;
	}
}


void commandMouseMove(POINT origin_point, LPTSTR arg) {
	origin_point.x += StrToInt(parseCommaSepArgUnescape(arg));
	origin_point.y += StrToInt(parseCommaSepArgUnescape(arg));
	SetCursorPos(origin_point.x, origin_point.y);
	sleepBackground(0);
}


void commandMouseWheel(LPTSTR arg) {
	const int offset = -StrToInt(arg) * WHEEL_DELTA;
	if (offset) {
		mouse_event(MOUSEEVENTF_WHEEL, 0, 0, static_cast<DWORD>(offset), 0);
		sleepBackground(0);
	}
}


void commandKeysDown(ExecutionContext* context, LPTSTR arg) {
	Keystroke keep_down_keystroke;
	keep_down_keystroke.parseDisplayName(arg);
	context->keep_down_mod_code = keep_down_keystroke.getUnsidedModCode();
	
	// Release the old special keys.
	Keystroke::releaseSpecialKeys(context->keyboard_state, /* keep_down_mod_code= */ 0);
	
	// Press the new special keys.
	for (int i = 0; i < arrayLength(e_special_keys); i++) {
		const SpecialKey& special_key = e_special_keys[i];
		if (context->keep_down_mod_code & special_key.mod_code) {
			// Press the left instance of the special key.
			BYTE vk_left = special_key.vk_left;
			Keystroke::keybdEvent(vk_left, /* down= */ true);
			context->keyboard_state[vk_left] = context->keyboard_state[special_key.vk] = keyDownMask;
		}
	}
}


//------------------------------------------------------------------------
// Matching
//------------------------------------------------------------------------

// Test for inclusion
bool Shortcut::isSubset(const Keystroke& other_ks, LPCTSTR other_program) const {
	VERIF(Keystroke::isSubset(other_ks));
	
	return (other_program && m_programs.isSome() && containsProgram(other_program)) ^ (!m_programs_only);
}

// Test for intersection
bool Shortcut::testConflict(
		const Keystroke& other_ks, const String other_programs[], bool other_programs_only) const {
	VERIF(other_programs_only == m_programs_only);
	
	VERIF(m_vk == other_ks.m_vk);
	
	if (m_sided && other_ks.m_sided) {
		VERIF(m_sided_mod_code == other_ks.m_sided_mod_code);
	} else {
		VERIF(getUnsidedModCode() == other_ks.getUnsidedModCode());
	}
	
	for (int i = 0; i < condTypeCount; i++) {
		VERIF(
			m_conditions[i] == condIgnore ||
			other_ks.m_conditions[i] == condIgnore ||
			m_conditions[i] == other_ks.m_conditions[i]);
	}
	
	if (!m_programs_only || !other_programs_only) {
		return true;
	}
	
	VERIF(other_programs && m_programs.isSome());
	for (int i = 0; other_programs[i].isSome(); i++) {
		if (containsProgram(other_programs[i])) {
			return true;
		}
	}
	return false;
}

String* Shortcut::getPrograms() const {
	VERIFP(m_programs.isSome(), nullptr);
	
	const LPCTSTR pszPrograms = m_programs;
	int nbProgram = 0;
	for (int i = 0; pszPrograms[i]; i++) {
		if (pszPrograms[i] == _T(';') && i > 0 && pszPrograms[i - 1] != _T(';')) {
			nbProgram++;
		}
	}
	String *const asProgram = new String[nbProgram + 2];
	nbProgram = 0;
	for (int i = 0; pszPrograms[i]; i++) {
		if (pszPrograms[i] == _T(';')) {
			if (i > 0 && pszPrograms[i - 1] != _T(';')) {
				nbProgram++;
			}
		} else {
			asProgram[nbProgram] += pszPrograms[i];
		}
	}
	
	return asProgram;
}

void Shortcut::cleanPrograms() {
	String *const programs = getPrograms();
	m_programs.empty();
	VERIFV(programs);
	
	for (int i = 0; programs[i].isSome(); i++) {
		for (int j = 0; j < i; j++) {
			if (!lstrcmpi(programs[i], programs[j])) {
				goto Next;
			}
		}
		if (m_programs.isSome()) {
			m_programs += _T(';');
		}
		m_programs += programs[i];
		
	Next:
		;
	}
	
	delete [] programs;
}

bool Shortcut::containsProgram(LPCTSTR program) const {
	LPCTSTR programs = m_programs;
	VERIF(*programs);
	
	const TCHAR* current_program_begin = programs;
	for (;;) {
		const TCHAR *current_process_end = current_program_begin;
		while (*current_process_end && *current_process_end != _T(';')) {
			current_process_end++;
		}
		
		if (CSTR_EQUAL == CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE,
				current_program_begin,
				static_cast<int>(current_process_end - current_program_begin), program, -1)) {
			return true;
		}
		
		if (!*current_process_end) {
			return false;
		}
		current_program_begin = current_process_end + 1;
	}
}


//------------------------------------------------------------------------
// Serialization
//------------------------------------------------------------------------

void loadShortcuts() {
	clearShortcuts();
	mergeShortcuts(e_ini_filepath);
}

void mergeShortcuts(LPCTSTR ini_filepath) {
	e_icon_visible = true;
	
	memcpy(e_column_widths, s_default_column_widths, sizeof(s_default_column_widths));
	
	const HANDLE file = CreateFile(
		ini_filepath,
		GENERIC_READ, FILE_SHARE_READ, /* lpSecurityAttributes= */ nullptr, OPEN_EXISTING,
		/* dwFlagsAndAttributes= */ 0, /* hTemplateFile= */ NULL);
	if (file == INVALID_HANDLE_VALUE) {
		if (GetLastError() != ERROR_FILE_NOT_FOUND) {
Error:
			messageBox(/* hwnd= */ NULL, ERR_LOADING_INI);
		}
		return;
	}
	
	DWORD file_size = GetFileSize(file, /* lpFileSizeHigh= */ nullptr);
	if (file_size == INVALID_FILE_SIZE) {
		goto Error;
	}
	
	BYTE *file_contents = new BYTE[file_size + 2];
	DWORD lenRead;
	const bool ok = ReadFile(file, file_contents, file_size, &lenRead, /* lpOverlapped= */ nullptr) && lenRead == file_size;
	CloseHandle(file);
	
	if (!ok) {
		delete [] file_contents;
		goto Error;
	}
	
	file_contents[file_size] = file_contents[file_size + 1] = 0;
	LPTSTR input;
	if (IsTextUnicode(file_contents, file_size, /* lpiResult= */ nullptr)) {
		input = reinterpret_cast<LPTSTR>(file_contents);
		if (*input == kUtf16LittleEndianBom)
			input++;
	} else {
		LPSTR ansi_contents = reinterpret_cast<LPSTR>(file_contents);
		const int buf = lstrlenA(ansi_contents) + 1;
		input = new TCHAR[buf];
		MultiByteToWideChar(CP_ACP, 0, ansi_contents, -1, input, buf);
		file_contents = reinterpret_cast<BYTE*>(input);
	}
	
	do {
		Shortcut *const shortcut = new Shortcut;
		if (shortcut->load(&input)) {
			shortcut->addToList();
			shortcut->registerHotKey();
		} else {
			delete shortcut;
		}
	} while (input);
	
	delete [] file_contents;
	HeapCompact(e_heap, 0);
}


void saveShortcuts() {
	HANDLE file;
	for (;;) {
		file = CreateFile(
			e_ini_filepath,
			GENERIC_WRITE, /* dwShareMode= */ 0, /* lpSecurityAttributes= */ nullptr, CREATE_ALWAYS,
			/* dwFlagsAndAttributes= */ 0, /* hTemplateFile= */ NULL);
		if (file != INVALID_HANDLE_VALUE) {
			break;
		}
		VERIFV(messageBox(/* hwnd= */ NULL, ERR_SAVING_INI, MB_ICONERROR | MB_RETRYCANCEL) == IDRETRY);
	}
	
	TCHAR buffer[1024];
	
	wsprintf(buffer, _T("%c%s=%s\r\n"),
		kUtf16LittleEndianBom, getToken(tokLanguage), getToken(tokLanguageName));
	writeFile(file, buffer);
	
	wsprintf(buffer, _T("%s=%d,%d,%d,%d\r\n"),
		getToken(tokSize), e_sizeMainDialog.cx, e_sizeMainDialog.cy,
		e_maximize_main_dialog, !e_icon_visible);
	writeFile(file, buffer);
	
	writeFile(file, getToken(tokColumns));
	for (int i = 0; i < kSizedColumnCount; i++) {
		wsprintf(buffer, (i == 0) ? _T("=%d") : _T(",%d"), e_column_widths[i]);
		writeFile(file, buffer);
	}
	
	wsprintf(buffer, _T("\r\n%s=%d\r\n\r\n"),
		getToken(tokSorting), Shortcut::s_sort_column);
	writeFile(file, buffer);
	
	for (Shortcut* shortcut = getFirst(); shortcut; shortcut = shortcut->getNext()) {
		shortcut->save(file);
	}
	
	CloseHandle(file);
}


void clearShortcuts() {
	while (s_first_shortcut) {
		Shortcut *const old_first_shortcut = s_first_shortcut;
		s_first_shortcut = old_first_shortcut->getNext();
		delete old_first_shortcut;
	}
	s_first_shortcut = nullptr;
}

}  // shortcut namespace
