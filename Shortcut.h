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

#include "Keystroke.h"

struct GETFILEICON;

namespace shortcut {

class Shortcut;

class GuardList {
public:
	GuardList() {
		EnterCriticalSection(&s_critical_section);
	}
	
	~GuardList() {
		LeaveCriticalSection(&s_critical_section);
	}
	
	static CRITICAL_SECTION s_critical_section;
};

class Shortcut : public Keystroke {
public:
	
	Shortcut(const Shortcut& sh);
	Shortcut(const Keystroke& ks);
	~Shortcut() {
		resetIcons();
	}
	
	void save(HANDLE hf);
	bool load(LPTSTR& rpszCurrent);
	
	bool execute(bool bFromHotkey) const;
	
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
	
	int getMatchingLevel() const {
		return m_matching_level;
	}
	
	void setMatchingLevel(int level) {
		m_matching_level = level;
	}
	
	// Computes and return the matching level of the shortcut against a keystroke and environment.
	// Does not store the matching level.
	int computeMatchingLevel(const Keystroke& ks, LPCTSTR pszProgram) const;
	
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
	
	static int s_iSortColumn;
	static int CALLBACK compare(const Shortcut* psh1, const Shortcut* psh2, LPARAM lParamSort);
	
	static bool tryChangeDirectory(HWND hdlg, LPCTSTR pszDirectory);
	
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
	bool m_bProgramsOnly;
	bool m_bSupportFileOpen;
	
	String m_sDescription;
	String m_sText;
	String m_sCommand;
	String m_sDirectory;
	String m_sPrograms;
	
private:
	
	bool containsProgram(LPCTSTR pszProgram) const;
	
private:
	
	Shortcut* m_next_shortcut;
	
	// The matching level of a shortcut against a multi-keystroke and its environment is not boolean,
	// because of inheritance. For example, when matching against "match.exe", a keystroke with
	// "these programs: match.exe" will have a higher matching level than a keystroke with only
	// "all programs but: none". The matching level is zero if the shortcut does not match, strictly
	// positive if it matches.
	int m_matching_level;
};

const int nbShowOption = 3;
extern const int aiShowOption[nbShowOption];

struct MATCHING_RESULT {
	int matching_shortcuts_count;
	int max_matching_level;  // < 0 of no shortcut matches
};

extern MATCHING_RESULT e_matching_result;

// Computes the matching level of all shortcuts against a keystroke and a program. Should not be
// called while the main dialog box is displayed. Should not be called concurrently. Stores the
// result in e_matching_result.
void computeAllMatchingLevels(const Keystroke& ks, LPCTSTR pszProgram);

// Finds the first shortcut having at least a given matching level, starting from a given shortcut
// in the linked list. Should not be called while the main dialog box is displayed.
//
// Args:
//   from_this: The shortcut from which to start the search. If it is marked, it will be returned.
//     Returns NULL if from_this is NULL.
//   min_matching_level: The minimum matching level of the shortcuts to return.
//
// Returns:
//   The first suitable shortcut from from_this, NULL if none is found.
Shortcut* getNextMatching(Shortcut* from_this, int min_matching_level);

// Initializes the namespace variables. Should be called once.
void initialize();

// Deletes the namespace variables. Should be called once.
void terminate();

// Returns the first shortcut of the linked list.
Shortcut* getFirst();

void loadShortcuts();
void mergeShortcuts(LPCTSTR pszIniFile);
void saveShortcuts();
void clearShortcuts();
void copyShortcutsToClipboard(const String& rs);

}  // shortcut namespace

using shortcut::Shortcut;
