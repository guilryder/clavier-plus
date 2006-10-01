// Clavier+
// Keyboard shortcuts manager
//
// Copyright (C) 2000-2006 Guillaume Ryder
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


#pragma once

#include "Global.h"

#define WM_KEYSTROKE   (WM_USER + 100)


enum
{
	condTypeShiftLock,
	condTypeNumLock,
	condTypeScrollLock,
	condTypeCount
};

enum
{
	condIgnore,
	condYes,
	condNo,
	condCount
};

class Keystroke
{
public:
	
	BYTE m_vk;
	WORD m_vkFlags;
	int  m_aCond[condTypeCount];
	
	
public:
	
	Keystroke()
	{
		resetAll();
	}
	
	void reset()
	{
		m_vk      = 0;
		m_vkFlags = 0;
	}
	
	void resetAll()
	{
		reset();
		for (int i = 0; i < condTypeCount; i++)
			m_aCond[i] = condIgnore;
	}
	
	
	ATOM getKeyName(LPTSTR pszHotKey, bool bGetAtom = false) const;
	
	void serialize(LPTSTR psz);
	void simulateTyping() const;
	
	void registerHotKey();
	bool unregisterHotKey();
	
	bool match(BYTE vk, WORD vkFlags, const int aiCondState[]) const;
	bool testConflict(const Keystroke& ks) const;
	
	bool testCanBeAdded() const;
	
	
	static bool isKeyExtended(UINT vk);
	static void getKeyName(UINT vk, LPTSTR pszHotKey);
	static void catchKeyboardFocus(HWND& rhwndFocus);
};


class Shortcut : public Keystroke
{
public:
	
	Shortcut *m_pNext;
	
	bool m_bCommand;
	int  m_nShow;
	
	String m_sDescription;
	String m_sText;
	String m_sCommand;
	String m_sDirectory;
	
	
public:
	
	Shortcut(const Keystroke& ks);
	
	void save(HANDLE hf);
	bool load(LPTSTR& rpszCurrent);
	
	void appendItemToString(String& rs) const;
	
	static int CALLBACK compare(const Shortcut* psh1, const Shortcut* psh2, LPARAM lParamSort);
	
	void appendMenuItem(HMENU hMenu, UINT id) const;
	bool execute() const;
};



bool AskKeystroke(HWND hwndParent, Shortcut* pksEdited, Keystroke& rksResult);




enum
{
	tokShortcut,
	tokCode,
	tokDescription,
	tokCommand,
	tokText,
	tokDirectory,
	tokWindow,
	tokNone,
	tokLanguage,
	tokSize,
	tokColumns,
	
	tokShowNormal,
	tokShowMinimize,
	tokShowMaximize,
	
	tokWin,
	tokCtrl,
	tokShift,
	tokAlt,
	
	tokConditionCapsLock,
	tokConditionNumLock,
	tokConditionScrollLock,
	tokConditionYes,
	tokConditionNo,
	
	tokNotFound
};


extern HWND e_hlst;      // Shortcut list

extern Shortcut *e_pshFirst;    // Shortcut list
