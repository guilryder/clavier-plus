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
	
	Shortcut(const Shortcut& sh);
	Shortcut(const Keystroke& ks);
	
	~Shortcut() {
		resetIcons();
	}
	
	void save(HANDLE hf);
	bool load(LPTSTR& rpszCurrent);
	
	bool execute(bool bFromHotkey);
	
	//------------------------------------------------------------------------
	// List
	//------------------------------------------------------------------------
	
	Shortcut* getNext() const {
		return m_next_shortcut;
	}
	
	void addToList();
	
	//------------------------------------------------------------------------
	// Matching
	//------------------------------------------------------------------------
	
	String* getPrograms() const;
	void cleanPrograms();
	
	bool match(const Keystroke& ks, LPCTSTR pszProgram) const;
	bool testConflict(const Keystroke& ks, const String asProgram[], bool bProgramsOnly) const;
	
	
protected:
	
	bool containsProgram(LPCTSTR pszProgram) const;
	
	//------------------------------------------------------------------------
	// GUI support
	//------------------------------------------------------------------------
	
public:
	
	void findExecutable(LPTSTR pszExecutable);
	void findSmallIconIndex();
	void findIcon();
	void resetIcons();
	
	void fillGetFileIcon(GETFILEICON* pgfi, bool bSmallIcon);
	
	int getSmallIconIndex();
	
	HICON getIcon() {
		if (!m_hIcon) {
			findIcon();
		}
		return m_hIcon;
	}
	
	void appendCsvLineToString(String& rs) const;
	
	void onGetFileInfo(GETFILEICON& gfi);
	
	void getColumnText(int iColumn, String& rs) const;
	
	// The column to sort the shortcuts list against. See col* enum defined in Global.h.
	static int s_iSortColumn;
	
	// Compares two shortcuts according to s_iSortColumn.
	//
	// Returns:
	//   A < 0 integer if shortcut1 is before shortcut2, > 0 if shortcut1 is after shortcut2,
	//   0 if the shortcuts are equal for the column.
	static int CALLBACK compare(const Shortcut* shortcut1, const Shortcut* shortcut2, LPARAM);
	
private:
	
	enum {
		iconInvalid = -1,
		iconNeeded = -2,
		iconThreadRunning = -3,
	};
	
	int m_iSmallIcon;
	HICON m_hIcon;
	
public:
	
	bool m_bCommand;
	int m_nShow;
	
	// If true, the program conditions (see m_sPrograms below) are positive: the shortcut must match
	// the condition to be executed. If false, the program conditions are negative: the shortcut must
	// match none of the conditions to be executed.
	bool m_bProgramsOnly;
	
	// Indicates whether the shortcut can change the current directory in File/Open
	bool m_bSupportFileOpen;
	
	String m_sDescription;
	String m_sText;
	String m_sCommand;
	String m_sDirectory;
	
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
	String m_sPrograms;
	
	// The number of times the shortcut has been used.
	int m_nbUsage;
	
private:
	
	Shortcut* m_next_shortcut;
};

const int nbShowOption = 3;
extern const int aiShowOption[nbShowOption];

// Initializes the namespace variables. Should be called once.
void initialize();

// Deletes the namespace variables. Should be called once.
void terminate();

// Returns the first shortcut of the linked list.
Shortcut* getFirst();

// Find a shortcut in the linked list
// Should not be called while the main dialog box is displayed
Shortcut* find(const Keystroke& ks, LPCTSTR program);

void loadShortcuts();
void mergeShortcuts(LPCTSTR pszIniFile);
void saveShortcuts();
void clearShortcuts();

}  // shortcut namespace

using shortcut::Shortcut;
