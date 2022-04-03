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

#include <functional>

#include "Resource.h"

#include "MyString.h"


namespace shortcut {

class Shortcut;

}  // namespace shortcut


constexpr size_t kStringBufSize = 128;
constexpr size_t kHotKeyBufSize = 128;
constexpr size_t kCodeBufSize = 32;
constexpr size_t kWindowTitleBufSize = 256;

constexpr int kClipboardStringBugSize = MAX_PATH * 10;

// Name of the environment variable set by clipboardToEnvironment().
constexpr LPCTSTR kClipboardEnvVariableName = _T("CLIPBOARD");


constexpr LPCTSTR kAppName = _T("Clavier+");


extern HANDLE e_heap;
extern HINSTANCE e_instance;
extern HWND e_invisible_window;  // Invisible background window
extern HWND e_modal_dialog;
extern bool e_icon_visible;


inline void subclassWindow(HWND hwnd, SUBCLASSPROC subclass_proc, DWORD_PTR ref_data = 0) {
	SetWindowSubclass(hwnd, subclass_proc, /* uIdSubclass= */ 0, ref_data);
}


// Display a message box. The message is read from string resources.
int messageBox(HWND hwnd, UINT string_id, UINT type = MB_ICONERROR, LPCTSTR arg = nullptr);

// Centers a window relatively to its parent.
void centerParent(HWND hwnd);

// Reads the text of a dialog box control.
void getDlgItemText(HWND hdlg, UINT id, String* output);

// Setups a label control for displaying an URL.
//
// Args:
//   hdlg: The handle of the dialog box containing the control to setup.
//   control_id: The ID of the label control in the dialog box.
//   link: The URL to give to the control. It must be static buffer: the string will not
//     be copied to a buffer.
void initializeWebLink(HWND hdlg, UINT control_id, LPCTSTR link);

// Wrapper for CreateThread().
void startThread(LPTHREAD_START_ROUTINE pfn, void* params);

// Writes a NULL-terminated string to a file.
void writeFile(HANDLE file, LPCTSTR strbuf);

// Retrieves the basename of the process that created a window.
// Returns true on success (process_name contains a valid name), false on failure.
//
// hwnd: The window to get the process name of.
// process_name: The buffer where to put the basename of the process that created the window,
//   in lowercase. Should have a size of MAX_PATH.
bool getWindowProcessName(HWND hwnd, LPTSTR process_name);

// Sleeps in idle priority.
//
// duration_millis: the time to sleep, in milliseconds.
void sleepBackground(DWORD duration_millis);

// Wrapper for SHGetFileInfo() that does not call the function if the file belongs
// to a slow device. If the call to SHGetFileInfo() fails and SHGFI_USEFILEATTRIBUTES was not
// specified in flags, the flag is added and SHGetFileInfo() is called again.
// Returns true on success.
//
// path: the path of the file or directory to call SHGetFileInfo() on.
// file_attributes: given as argument to SHGetFileInfo().
// shfi: where to save the result of SHGetFileInfo().
// flags: given as argument to SHGetFileInfo(). If path is on a slow device,
//   according pathIsSlow(), SHGFI_USEFILEATTRIBUTES is added to flags before it is given
//   to SHGetFileInfo().
bool getFileInfo(LPCTSTR path, DWORD file_attributes, SHFILEINFO& shfi, UINT flags);

// Puts the text contents of the clipboard to the environment variable named clipboard_env_variable.
void clipboardToEnvironment();

// Returns a visible child of a window matching the given window class name, NULL if none is found.
// Picks an arbitrary window if multiple match.
//
// hwnd_parent: the parent to consider the child window of.
// wnd_class: the window class the returned window should match.
// allow_same_prefix: if false, the returned window will have exactly wnd_class as class name.
//   If true, it will have wnd_class only as prefix; full equality is not enforced.
HWND findVisibleChildWindow(HWND hwnd_parent, LPCTSTR wnd_class, bool allow_same_prefix);

// Determines whether a window has a window class, or a prefix of it.
//
// hwnd: the window to check the class of.
// wnd_class: the class the window should have.
// allow_same_prefix: if false, the window should have exactly wnd_class as class name to match.
//   If true, it should only have wnd_class only as prefix; full equality is not enforced.
bool checkWindowClass(HWND hwnd, LPCTSTR wnd_class, bool allow_same_prefix);

// Returns a top-level window whose title matches a regexp, NULL if none is found.
// Picks an arbitrary window if multiple match.
//
// title_regexp: the regular expression the title of the returned window will match.
//   The regular expression is evaluated by matchWildcards().
HWND findWindowByName(LPCTSTR title_regexp);

// Determines if a string matches a wildcards pattern. Case insensitive.
//
// pattern: the pattern to use for testing matching. Supports '*' and '?'.
//   This string must be given in lower case.
// subject: the string to test against the pattern. The case does not matter.
// pattern_end: if not null, points to the successor of the last character of the pattern; if
//   equals pattern, the pattern will be considered empty.
bool matchWildcards(LPCTSTR pattern, LPCTSTR subject, LPCTSTR pattern_end = nullptr);

// Copies text into the clipboard.
void setClipboardText(LPCTSTR text);


// SHBrowseForFolder wrapper:
// - use custom title
// - set initial directory
// - return directory path instead of LPITEMIDLIST
bool browseForFolder(HWND hwnd_parent, LPCTSTR title, LPTSTR directory);


// Changes the current directory in the given dialog box.
// Supported dialog boxes:
// - Standard Windows File/Open and similar: GetOpenFileName, GetSaveFileName
// - MS Office File/Open and similar
// - Standard Windows folder selection: SHBrowseForFolder
//
// Args:
//   hwnd: The window handle of the dialog box or one of its controls.
//   directory: The directory to set in the dialog box.
//
// Returns:
//   True if the dialog box is supported and the current directory has been changed.
bool setDialogBoxDirectory(HWND hwnd, LPCTSTR directory);


//------------------------------------------------------------------------
// Shell API tools
//------------------------------------------------------------------------

// Wrapper for SHGetSpecialFolderPath
bool getSpecialFolderPath(int csidl, LPTSTR path);

// Populates a shortcut with the target of a link file.
// Handles special cases natively:
// - installer shortcuts (MSI API)
// - IUniformResourceLocator shortcuts
// - IShellLink shortcuts
// Does not natively handle shortcuts to UWP apps.
void resolveLinkFile(LPCTSTR link_file, shortcut::Shortcut* shortcut);

// Invokes the callback for each UWP app found, by display name order.
void listUwpApps(std::function<void(LPCTSTR name, LPCTSTR app_id, LPITEMIDLIST pidl)> app_callback);


//------------------------------------------------------------------------
// Application-specific helpers
//------------------------------------------------------------------------

// Each token is associated to a part of the IDS_TOKENS resource string. IDS_TOKENS is a ';'
// separated string whose parts are in the same other as the values of this enum.
enum class Token {
	kFirst,
	kLanguageName = kFirst,
	
	kShortcut,
	kCode,
	kDistinguishLeftRight,
	kDescription,
	kCommand,
	kText,
	kDirectory,
	kWindow,
	kSupportFileOpen,
	kPrograms,
	kAllProgramsBut,
	kLanguage,
	kSize,
	kColumns,
	kSorting,
	
	// Matches Shortcut::kShowOptions.
	kShowNormal,
	kShowMinimize,
	kShowMaximize,
	
	kWin,
	kCtrl,
	kShift,
	kAlt,
	kLeft,
	kRight,
	
	// Matches Keystroke::CondType.
	kConditionCapsLock,
	kConditionNumLock,
	kConditionScrollLock,
	
	// Matches Keystroke::Condition, except kIgnore.
	kConditionYes,
	kConditionNo,
	
	kUsageCount,
	
	kNotFound
};

inline Token operator +(Token tok, int delta) {
	return static_cast<Token>(static_cast<int>(tok) + delta);
}

inline int operator -(Token tok1, Token tok2) {
	return static_cast<int>(tok1) - static_cast<int>(tok2);
}

inline Token operator ++(Token& tok, int) {
	const Token copy = tok;
	tok = tok + 1;
	return copy;
}

// Returns the translation of a token in the current language.
LPCTSTR getToken(Token tok);

// Returns the name of a language in this language.
LPCTSTR getLanguageName(i18n::Language lang);

// Returns the index of a token from its name. The name of the token can be in any language.
//
// Returns:
//   A kTok* enum value, or kTokNotFound if the token does not match any token.
Token findToken(LPCTSTR token);


//------------------------------------------------------------------------
// Strings
//------------------------------------------------------------------------

// '\'-unescapes the given string.
// In-place is safe because the unescaping process can only but reduce the string length.
void unescape(LPTSTR str);

// Increments a string pointer until the end of the string or a comma is encountered, then skips
// spaces. The comma is replaced by a null character, so that the initial pointer (before being
// incremented) will point to the string before the comma.
//
// Args:
//   chr_ptr: A reference to the pointer to increment. The string initially pointed will be
//     unescaped if unescape is true.
//   unescape: Whether to recognize '\'-escaping when looking for the comma
//     and unescape the returned string (up to the comma).
//
// Returns the original value of chr_ptr.
LPCTSTR parseCommaSepArg(TCHAR*& chr_ptr, bool unescape = false);

inline LPCTSTR parseCommaSepArgUnescape(TCHAR*& chr_ptr) {
	return parseCommaSepArg(chr_ptr, /* unescape= */ true);
}

// Isolates the next ';'-separated token with a '\0'.
// Returns token_start, then modifies token_start to point to the start of the next token.
LPCTSTR getSemiColonToken(LPTSTR* token_start);


//------------------------------------------------------------------------
// Strings translation
//------------------------------------------------------------------------

#include "I18n.h"

// A string translated in all available languages.
class TranslatedString {
public:
	
	TranslatedString() = default;
	TranslatedString(const TranslatedString& other) = delete;
	TranslatedString& operator =(const TranslatedString& other) = delete;
	
	// Loads the translation of the string for the current language.
	void load(UINT id) {
		m_translations[i18n::getLanguage()].loadString(id);
	}
	
	// Sets the translation of the string for the current language.
	void set(LPCTSTR strbuf) {
		m_translations[i18n::getLanguage()] = strbuf;
	}
	
	// Returns the translation of the string for a given language,
	// an empty string when the translation should use the default.
	LPCTSTR get(int lang) const {
		return m_translations[lang];
	}
	
	// Returns the translation of the string for the current language.
	// Falls back to the default default if the translation is empty.
	LPCTSTR get() const {
		LPCTSTR translation = get(i18n::getLanguage());
		return *translation ? translation : get(i18n::kLangDefault);
	}
	
private:
	
	// Translations of the string, indexed by language.
	String m_translations[i18n::kLangCount];
};


//------------------------------------------------------------------------
// Command line parsing and executing
//------------------------------------------------------------------------

void findFullPath(LPTSTR path, LPTSTR full_path);

void shellExecuteCmdLine(LPCTSTR command, LPCTSTR directory, int show_mode);


class ShellExecuteThread {
public:
	
	ShellExecuteThread(const String& command, const String& directory, int show_mode)
		: m_command(command), m_directory(directory), m_show_mode(show_mode) {}
	
	ShellExecuteThread(const ShellExecuteThread& other) = delete;
	ShellExecuteThread& operator =(const ShellExecuteThread& other) = delete;
	
	static DWORD WINAPI thread(void* params);
	
private:
	
	String m_command;
	String m_directory;
	int m_show_mode;
};


//------------------------------------------------------------------------
// Settings loading and saving
//------------------------------------------------------------------------

enum {
	kColContents,
	kColKeystroke,
	kColCond,
	kColUsageCount,
	kColDescription,
	kColCount
};

// In dialog units, indexed by the column enum.
constexpr int kDefaultColumnWidths[] = {
	35,  // kColContents
	20,  // kColKeystroke
	15,  // kColCond
	10,  // kColUsageCount
	// kColDescription takes the remaining space.
};


// Number of columns with an explicit size. The last column takes all the remaining space.
constexpr int kSizedColumnCount = kColCount - 1;

extern int e_column_widths[kColCount];

extern SIZE e_main_dialog_size;
extern bool e_maximize_main_dialog;

extern TCHAR e_ini_filepath[MAX_PATH];


//------------------------------------------------------------------------
// Autostart setting
//------------------------------------------------------------------------

bool isAutoStartEnabled();
void setAutoStartEnabled(bool enabled);
