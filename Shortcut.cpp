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
namespace {

Shortcut* s_first_shortcut;
Shortcut* s_last_shortcut;

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
void commandEmpty(ExecutionContext* context);

// [{Wait,duration}]
// Sleep for a given number of milliseconds.
void commandWait(LPTSTR arg);

// [{Focus,delay,[!]window_name}]
// Sleep for delay milliseconds and catch the focus.
// If window_name does not begin with '!', return false if the window is not found.
// Reads & updates input_thread and input_window in the context.
bool commandFocus(ExecutionContext* context, LPTSTR arg);

// [{FocusOrLaunch,window_name,command,delay}]
// Activate window_name.
// If the window is not found, relases any pressed special keys, execute command, then sleep for delay milliseconds.
// Either way, catch the focus (reads & updates input_thread and input_window in the context).
void commandFocusOrLaunch(ExecutionContext* context, LPTSTR arg);

// [{Copy,text}]
// Copy the text argument to the clipboard.
void commandCopy(LPTSTR arg);

// [{Mouse,state}] where state is 2 letters:
// 1 letter for button: L (left), M (middle), R (right)
// 1 letter for state: U (up), D (down)
// Simulate mouse clicks.
void commandMouseButton(LPTSTR arg);

// [{MouseMoveTo,x,y}], [{MouseMoveToFocus,x,y}], [{MouseMoveBy,dx,dy}]
// Move the mouse cursor.
// arg: move coordinates relative to origin_point.
void commandMouseMove(POINT origin_point, LPTSTR arg);

// [{MouseWheel,offset}]
// Simulate a mouse wheel scroll.
void commandMouseWheel(LPTSTR arg);

// [{KeysDown,keystroke}]
// Keep special keys down.
// Reads & updates keep_down_unsided_mod_code.
void commandKeysDown(ExecutionContext* context, LPTSTR arg);


// Return whether to continue executing the shortcut.
bool executeSpecialCommand(LPCTSTR shortcut_start, LPCTSTR& shortcut_end, ExecutionContext* context);

void executeCommandLine(LPCTSTR command, ExecutionContext* context);

void simulateCharacter(TCHAR c, DWORD keep_down_mod_code);

void focusWindow(HWND hwnd);


constexpr LPCTSTR kLineSeparator = _T("-\r\n");
constexpr int kConfigCodeModCodeOffset = 8;

}  // namespace


void initialize() {
	s_first_shortcut = s_last_shortcut = nullptr;
}

void terminate() {
}

Shortcut* getFirst() {
	return s_first_shortcut;
}

Shortcut* find(const Keystroke& ks, LPCTSTR program) {
	Shortcut *best_shortcut = nullptr;
	for (Shortcut* sh = getFirst(); sh; sh = sh->getNext()) {
		if (sh->isSubset(ks, program)) {
			// Pick the first matching shortcut, but give precedence to m_programs_only = true.
			if (!best_shortcut || (!best_shortcut->m_programs_only && sh->m_programs_only)) {
				best_shortcut = sh;
			}
		}
	}
	return best_shortcut;
}


Shortcut::Shortcut(const Shortcut& sh)
	: Keystroke(sh),
	m_type(sh.m_type),
	m_show_option(sh.m_show_option),
	m_programs_only(sh.m_programs_only),
	
	m_description(sh.m_description),
	m_text(sh.m_text),
	m_command(sh.m_command),
	m_directory(sh.m_directory),
	
	m_programs(sh.m_programs),
	m_usage_count(sh.m_usage_count),
	
	m_next_shortcut(nullptr),
	
	m_small_icon_index(sh.m_small_icon_index),
	m_icon(CopyIcon(sh.m_icon)) {}

Shortcut::Shortcut(const Keystroke& ks)
	: Keystroke(ks),
	m_type(Type::kText),
	m_show_option(SW_NORMAL),
	m_programs_only(false),
	
	m_usage_count(0),
	
	m_next_shortcut(nullptr),
	
	m_small_icon_index(kIconNeeded),
	m_icon(NULL) {}


void Shortcut::addToList() {
	if (s_last_shortcut == nullptr) {
		s_first_shortcut = this;
	} else {
		s_last_shortcut->m_next_shortcut = this;
	}
	s_last_shortcut = this;
}


void Shortcut::save(HANDLE file) {
	cleanPrograms();
	
	TCHAR display_name[kHotKeyBufSize], code_text[kCodeBufSize];
	getDisplayName(display_name);
	wsprintf(code_text, _T("%lu"), DWORD(m_vk) | (m_sided_mod_code << kConfigCodeModCodeOffset));
	
	struct Line {
		Token key_token;
		LPCTSTR value;
	};
	Line lines[3 + 4 + 2 + kCondTypeCount + 1];
	lines[0] = { .key_token = Token::kShortcut, .value = display_name };
	lines[1] = { .key_token = Token::kCode, .value = code_text };
	int line_count = 2;
	
	if (m_sided) {
		lines[line_count++] = { .key_token = Token::kDistinguishLeftRight, .value = _T("1") };
	}
	
	for (int i = 0; i < kCondTypeCount; i++) {
		if (m_conditions[i] != Condition::kIgnore) {
			lines[line_count++] = {
				.key_token = Token::kConditionCapsLock + i,
				.value = getToken(Token::kConditionYes + (int(m_conditions[i]) - int(Condition::kYes))),
			};
		}
	}
	
	String text;
	
	switch (m_type) {
		case Type::kCommand:
			lines[line_count++] = { .key_token = Token::kCommand, .value = m_command };
			
			if (m_directory.isSome()) {
				lines[line_count++] = { .key_token = Token::kDirectory, .value = m_directory };
			}
			
			lines[line_count] = { .key_token = Token::kWindow, .value = _T("") };
			for (int i = 0; i < arrayLength(kShowOptions); i++) {
				if (m_show_option == kShowOptions[i]) {
					lines[line_count].value = getToken(Token::kShowNormal + i);
					break;
				}
			}
			line_count++;
			break;
		
		case Type::kText:
			// Copy the text, replace "\r\n" by "\r\n>"
			// to handle multiple lines text
			for (const TCHAR *from = m_text; *from; from++) {
				text += *from;
				if (from[0] == _T('\r') && from[1] == _T('\n')) {
					text += _T("\n>");
					from++;
				}
			}
			
			lines[line_count++] = { .key_token = Token::kText, .value = text };
			break;
	}
	
	if (m_programs.isSome()) {
		lines[line_count++] = {
			.key_token = (m_programs_only) ? Token::kPrograms : Token::kAllProgramsBut,
			.value = m_programs,
		};
	}
	
	if (m_description.isSome()) {
		lines[line_count++] = { .key_token = Token::kDescription, .value = m_description };
	}
	
	TCHAR strbuf_usage_count[i18n::kIntegerBufSize];
	wsprintf(strbuf_usage_count, _T("%d"), m_usage_count);
	lines[line_count++] = { .key_token = Token::kUsageCount, .value = strbuf_usage_count };
	
	// Write all
	for (int i = 0; i < line_count; i++) {
		writeFile(file, getToken(lines[i].key_token));
		writeFile(file, _T("="));
		writeFile(file, lines[i].value);
		writeFile(file, _T("\r\n"));
	}
	writeFile(file, kLineSeparator);
}

bool Shortcut::load(LPTSTR* input) {
	// Read
	
	Token key_tok = Token::kNotFound;
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
		if (*line_start == _T('>') && key_tok == Token::kText) {
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
		key_tok = findToken(line_start);
		if (key_tok == Token::kNotFound) {
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
		switch (key_tok) {
			
			// Language
			case Token::kLanguage:
				for (int lang_index = 0; lang_index < i18n::kLangCount; lang_index++) {
					i18n::Language lang = i18n::Language(lang_index);
					if (!lstrcmpi(next_sep, getLanguageName(lang))) {
						i18n::setLanguage(lang);
					}
				}
				break;
			
			// Main window size
			case Token::kSize:
				while (*next_sep == _T(' ')) {
					next_sep++;
				}
				e_main_dialog_size = {
					.cx = StrToInt(parseCommaSepArg(next_sep)),
					.cy = StrToInt(parseCommaSepArg(next_sep)),
				};
				e_maximize_main_dialog = toBool(StrToInt(parseCommaSepArg(next_sep)));
				e_icon_visible = !StrToInt(parseCommaSepArg(next_sep));
				break;
			
			// Main window columns width
			case Token::kColumns:
				for (int i = 0; i < kColCount - 1; i++) {
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
			case Token::kSorting:
				s_sort_column = StrToInt(next_sep);
				if (s_sort_column < 0 || s_sort_column >= kColCount) {
					s_sort_column = kColContents;
				}
				break;
			
			// Shortcut
			case Token::kShortcut:
				Keystroke::parseDisplayName(next_sep);
				break;
			
			// Code
			case Token::kCode:
				if (!m_vk) {
					const DWORD dwCode = DWORD(StrToInt(next_sep));
					if (dwCode) {
						m_vk = canonicalizeKey(LOBYTE(dwCode));
						m_sided_mod_code = dwCode >> kConfigCodeModCodeOffset;
					}
				}
				break;
			
			// Distinguish left/right
			case Token::kDistinguishLeftRight:
				m_sided = toBool(StrToInt(next_sep));
				break;
			
			// Description
			case Token::kDescription:
				m_description = next_sep;
				break;
			
			// Text
			case Token::kText:
				m_type = Type::kText;
				m_text = next_sep;
				break;

			// Command
			case Token::kCommand:
				m_type = Type::kCommand;
				m_command = next_sep;
				break;
			
			// Directory
			case Token::kDirectory:
				m_directory = next_sep;
				break;
			
			// Programs
			case Token::kPrograms:
			case Token::kAllProgramsBut:
				m_programs = next_sep;
				m_programs_only = (key_tok == Token::kPrograms);
				cleanPrograms();
				break;
			
			// Window
			case Token::kWindow: {
				const int show_option_index = findToken(next_sep) - Token::kShowNormal;
				if (0 <= show_option_index && show_option_index < arrayLength(kShowOptions)) {
					m_show_option = kShowOptions[show_option_index];
				}
				break;
			}
			
			// Condition
			case Token::kConditionCapsLock:
			case Token::kConditionNumLock:
			case Token::kConditionScrollLock: {
				Token cond_tok = findToken(next_sep);
				if (Token::kConditionYes <= cond_tok && cond_tok <= Token::kConditionNo) {
					m_conditions[key_tok - Token::kConditionCapsLock] =
						Condition(int(Condition::kYes) + (cond_tok - Token::kConditionYes));
				}
				break;
			}
			
			// Usage count
			case Token::kUsageCount:
				m_usage_count = std::max(0, StrToInt(next_sep));
				break;
			
			// Ignore the other tokens.
			default:
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
	for (Shortcut* sh = getFirst(); sh; sh = sh->getNext()) {
		if (sh->testConflict(*this, programs, m_programs_only)) {
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
		if ((context.keyboard_state[VK_MENU] & kKeyDownMask) &&
				(context.keyboard_state[VK_SHIFT] & kKeyDownMask) &&
				!(context.keyboard_state[VK_CONTROL] & kKeyDownMask)) {
			release_control = true;
			keybdEvent(VK_CONTROL, /* down= */ true);
			new_keyboard_state[VK_CONTROL] = kKeyDownMask;
		}
		
		for (const auto& special_key : kSpecialKeys) {
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
		case Type::kCommand: {
			// Required because the launched program
			// can be a script that simulates keystrokes
			if (can_release_special_keys) {
				releaseSpecialKeys(context.keyboard_state, /* keep_down_mod_code= */ 0);
			}
			
			clipboardToEnvironment();
			ShellExecuteThread *const shell_execute_thread =
				new ShellExecuteThread(m_command, m_directory, m_show_option);
			startThread(shell_execute_thread->thread, shell_execute_thread);
			break;
		}
		
		case Type::kText:
			// Special keys to keep down across commands.
			
			LastTextExecution lastTextExecution = LastTextExecution::None;
			
			// Send the text to the window
			bool escaping = false;  // whether the next character is '\'-escaped
			LPCTSTR text = m_text;
			for (size_t i = 0; text[i]; i++) {
				const WORD c = WORD(text[i]);
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

namespace {

bool executeSpecialCommand(LPCTSTR shortcut_start, LPCTSTR& shortcut_end, ExecutionContext* context) {
	String inside(shortcut_start, int(shortcut_end - shortcut_start));
	
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
		const BYTE scan_code_alt = BYTE(MapVirtualKey(VK_MENU, MAPVK_VK_TO_VSC));
		keybd_event(VK_MENU, scan_code_alt, 0, 0);
		
		// Press the digits.
		TCHAR digits[5];
		wsprintf(digits, _T("0%u"), WORD(c));
		ks.m_sided_mod_code = MOD_ALT;
		for (size_t digit_index = 0; digits[digit_index]; digit_index++) {
			ks.m_vk = VK_NUMPAD0 + BYTE(digits[digit_index] - _T('0'));
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
		mouse_event(MOUSEEVENTF_WHEEL, 0, 0, DWORD(offset), 0);
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
	for (const auto& special_key : kSpecialKeys) {
		if (context->keep_down_mod_code & special_key.mod_code) {
			// Press the left instance of the special key.
			BYTE vk_left = special_key.vk_left;
			Keystroke::keybdEvent(vk_left, /* down= */ true);
			context->keyboard_state[vk_left] = context->keyboard_state[special_key.vk] = kKeyDownMask;
		}
	}
}

}  // namespace


//------------------------------------------------------------------------
// Matching
//------------------------------------------------------------------------

// Test for inclusion
bool Shortcut::isSubset(const Keystroke& other_ks, LPCTSTR other_program) const {
	VERIF(Keystroke::isSubset(other_ks));
	
	return (other_program && containsProgram(other_program)) == m_programs_only;
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
	
	for (int i = 0; i < kCondTypeCount; i++) {
		VERIF(
			m_conditions[i] == Condition::kIgnore ||
			other_ks.m_conditions[i] == Condition::kIgnore ||
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
	
	// Count the programs.
	int program_count = 0;
	const LPCTSTR programs_chr = m_programs;
	for (int i = 0; programs_chr[i]; i++) {
		if (programs_chr[i] == _T(';') && i > 0 && programs_chr[i - 1] != _T(';')) {
			program_count++;
		}
	}
	String *const programs = new String[program_count + 2];
	program_count = 0;
	for (int i = 0; programs_chr[i]; i++) {
		if (programs_chr[i] == _T(';')) {
			if (i > 0 && programs_chr[i - 1] != _T(';')) {
				program_count++;
			}
		} else {
			programs[program_count] += programs_chr[i];
		}
	}
	
	return programs;
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
				current_program_begin, int(current_process_end - current_program_begin), program, -1)) {
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
	
	memcpy(e_column_widths, kDefaultColumnWidths, sizeof(kDefaultColumnWidths));
	
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
		kUtf16LittleEndianBom, getToken(Token::kLanguage), getToken(Token::kLanguageName));
	writeFile(file, buffer);
	
	wsprintf(buffer, _T("%s=%d,%d,%d,%d\r\n"),
		getToken(Token::kSize), e_main_dialog_size.cx, e_main_dialog_size.cy,
		e_maximize_main_dialog, !e_icon_visible);
	writeFile(file, buffer);
	
	writeFile(file, getToken(Token::kColumns));
	for (int i = 0; i < kSizedColumnCount; i++) {
		wsprintf(buffer, (i == 0) ? _T("=%d") : _T(",%d"), e_column_widths[i]);
		writeFile(file, buffer);
	}
	
	wsprintf(buffer, _T("\r\n%s=%d\r\n\r\n"),
		getToken(Token::kSorting), Shortcut::s_sort_column);
	writeFile(file, buffer);
	
	for (Shortcut* sh = getFirst(); sh; sh = sh->getNext()) {
		sh->save(file);
	}
	
	CloseHandle(file);
}


void clearShortcuts() {
	Shortcut *sh = getFirst();
	while (sh) {
		Shortcut *const copy = sh;
		sh = sh->getNext();
		delete copy;
	}
	
	s_first_shortcut = s_last_shortcut = nullptr;
}

}  // namespace shortcut
