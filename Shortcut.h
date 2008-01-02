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

class Shortcut : public Keystroke {
public:
	
	Shortcut* m_pNext;
	
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
	
	enum {
		iconInvalid = -1,
		iconNeeded = -2,
		iconThreadRunning = -3,
	};
	
	int m_iSmallIcon;
	HICON m_hIcon;
	
	bool containsProgram(LPCTSTR pszProgram) const;
	
public:
	
	Shortcut(const Shortcut& sh);
	Shortcut(const Keystroke& ks);
	~Shortcut() {
		resetIcons();
	}
	
	void save(HANDLE hf);
	bool load(LPTSTR& rpszCurrent);
	
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
	
	void appendItemToString(String& rs) const;
	
	void onGetFileInfo(GETFILEICON& gfi);
	
	void getColumnText(int iColumn, String& rs) const;
	
	static int s_iSortColumn;
	static int CALLBACK compare(const Shortcut* psh1, const Shortcut* psh2, LPARAM lParamSort);
	
	bool execute(bool bFromHotkey) const;
	
	bool match(const Keystroke& ks, LPCTSTR pszProgram) const;
	bool testConflict(const Keystroke& ks, const String asProgram[], bool bProgramsOnly) const;
	
	String* getPrograms() const;
	void cleanPrograms();
	
	static bool tryChangeDirectory(HWND hdlg, LPCTSTR pszDirectory);
};


extern Shortcut *e_pshFirst;  // Head of the shortcuts linked list

const int nbShowOption = 3;
extern const int aiShowOption[nbShowOption];

Shortcut* find(const Keystroke& ks, LPCTSTR pszProgram);

void loadShortcuts();
void mergeShortcuts(LPCTSTR pszIniFile);
void saveShortcuts();
void clearShortcuts();
void copyShortcutsToClipboard(const String& rs);

}  // shortcut namespace

using shortcut::Shortcut;
