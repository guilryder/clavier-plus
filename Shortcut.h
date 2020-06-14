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

#include "Keystroke.h"

struct GETFILEICON;

namespace shortcut {

class Shortcut : public Keystroke {
public:
	
	explicit Shortcut(const Shortcut& sh);
	explicit Shortcut(const Keystroke& ks);
	
	~Shortcut() {
		clearIcons();
	}
	
	void save(HANDLE file);
	
	// Assumes the shortcut is initially empty.
	bool load(LPTSTR* input);
	
	void execute(bool from_hotkey);
	
	Shortcut* getNext() const {
		return m_next_shortcut;
	}
	
	void addToList();
	
	// Returns the array of programs of this shortcut, with the last element nullptr,
	// or nullptr if no programs.
	String* getPrograms() const;
	
	// Removes duplicates from getPrograms().
	void cleanPrograms();
	
	// Returns whether this shortcut would be a subset of a shortcut having the given attributes.
	bool isSubset(const Keystroke& other_ks, LPCTSTR other_program) const;
	
	// Returns whether this shortcut would conflict (overlap) with a shortcut having the given attributes.
	bool testConflict(const Keystroke& other_ks, const String other_programs[], bool other_programs_only) const;
	
private:
	
	// Returns whether getPrograms() contains the given entry. Case insentitive.
	bool containsProgram(LPCTSTR program) const;
	
public:
	
	void findExecutable(LPTSTR executable);
	void findSmallIconIndex();
	void findIcon();
	void clearIcons();
	
	void fillGetFileIcon(GETFILEICON* pgfi, bool small_icon);
	
	int getSmallIconIndex();
	
	HICON getIcon() {
		if (!m_icon) {
			findIcon();
		}
		return m_icon;
	}
	
	// Appends the shortcut CSV representation to the given string.
	void appendCsvLineToString(String& output) const;
	
	void onGetFileInfo(GETFILEICON& gfi);
	
	void getColumnText(int column_index, String& output) const;
	
	// The column to sort the shortcuts list against. See col* enum defined in Global.h.
	static int s_sort_column;
	
	// Compares two shortcuts according to s_sort_column.
	//
	// Returns:
	//   A < 0 integer if shortcut1 is before shortcut2, > 0 if shortcut1 is after shortcut2,
	//   0 if the shortcuts are equal for the column.
	static int CALLBACK compare(const Shortcut* shortcut1, const Shortcut* shortcut2, LPARAM lParam);
	
private:
	
	// Special values for m_small_icon_index.
	enum {
		iconInvalid = -1,
		iconNeeded = -2,
		iconThreadRunning = -3,
	};
	
	int m_small_icon_index;
	HICON m_icon;
	
public:
	
	enum class Type { Text, Command };
	
	Type m_type;
	
	// One of show_options. Applies to Type::Command shortcuts only.
	int m_show_option;
	
	// If true, the program conditions (see m_sPrograms below) are positive: the shortcut must match
	// the condition to be executed. If false, the program conditions are negative: the shortcut must
	// match none of the conditions to be executed.
	bool m_programs_only;
	
	// Indicates whether the shortcut can change the current directory in File/Open
	bool m_support_file_open;
	
	String m_description;
	String m_text; // Type::Text only
	String m_command; // Type::Command only
	String m_directory;
	
	// The list of program conditions, as a ';' separated string. The whole condition is satisfied if
	// any of the sub-conditions matches. The matching result is reversed if m_bProgramsOnly is false.
	//
	// Each condition has the following format: "process_name" or "process_name:window_title".
	// "process_name" is a process executable basename, such as explorer.exe. If empty, all processes
	// match. "window_title" is a wildcards expression (see matchWildcards function) matching the
	// window title. If empty, all windows match.
	//
	// Example: "explorer.exe;Test*;firefox.exe" matches Explorer windows whose title starts with
	// "Test" and all Firefox windows.
	String m_programs;
	
	// The number of times the shortcut has been used.
	int m_usage_count;
	
private:
	
	Shortcut* m_next_shortcut;
};

constexpr int show_options[] = { SW_NORMAL, SW_MINIMIZE, SW_MAXIMIZE };

// Initializes the namespace variables. Should be called once.
void initialize();

// Deletes the namespace variables. Should be called once.
void terminate();

// Returns the first shortcut of the linked list.
Shortcut* getFirst();

// Find a shortcut in the linked list
// Should not be called while the main dialog box is displayed
Shortcut* find(const Keystroke& ks, LPCTSTR program);

// Loads the list of shortcuts from e_pszIniFile.
void loadShortcuts();

// Merges the shortcuts of e_pszIniFile into the current list of shortcuts.
void mergeShortcuts(LPCTSTR pszIniFile);

// Saves the list of shortcuts into e_pszIniFile.
void saveShortcuts();

// Clears the list of shortcuts.
void clearShortcuts();

}  // shortcut namespace

using shortcut::Shortcut;
