// Clavier+
// Keyboard shortcuts manager
//
// Copyright (C) 2000-2007 Guillaume Ryder
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

#include "App.h"

#define IsKeyDown(vk)   (GetKeyState(vk) & 0x8000)
const BYTE bKeyDown = 0x80;


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
	void simulateTyping(HWND hwndFocus, bool bSpecialKeys = true) const;
	
	void registerHotKey();
	bool unregisterHotKey();
	
	bool match(BYTE vk, WORD vkFlags, const int aiCondState[]) const;
	
	bool canReleaseSpecialKeys() const
	{
		return (0xA6 > m_vk || m_vk > 0xB7);
	}
	
	static bool askSendKeys(HWND hwndParent, Keystroke& rks);
	
	
	static void keybdEvent(UINT vk, bool bUp);
	static BYTE filterVK(BYTE vk) { return (vk == VK_CLEAR) ? VK_NUMPAD5 : vk; }
	static bool isKeyExtended(UINT vk);
	static void getKeyName(UINT vk, LPTSTR pszHotKey);
	static HWND getKeyboardFocus();
	static void catchKeyboardFocus(HWND& rhwndFocus, DWORD& ridThread);
	static void detachKeyboardFocus(DWORD idThread);
	static void releaseSpecialKeys(BYTE abKeyboard[]);
	
private:
	
	static INT_PTR CALLBACK prcSendKeys(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};



class Shortcut;

struct GETFILEICON
{
	Shortcut  *psh;
	SHFILEINFO shfi;
	TCHAR      pszExecutable[MAX_PATH];
	UINT       uFlags;
	bool       bOK;
	bool       bAutoDelete;
};


class Shortcut : public Keystroke
{
public:
	
	Shortcut *m_pNext;
	
	bool m_bCommand;
	int  m_nShow;
	bool m_bProgramsOnly;
	bool m_bSupportFileOpen;
	
	String m_sDescription;
	String m_sText;
	String m_sCommand;
	String m_sDirectory;
	String m_sPrograms;
	
	
protected:
	
	enum
	{
		iconInvalid       = -1,
		iconNeeded        = -2,
		iconThreadRunning = -3,
	};
	
	int   m_iSmallIcon;
	HICON m_hIcon;
	
	bool containsProgram(LPCTSTR pszProgram) const;
	
	
public:
	
	Shortcut(const Keystroke& ks);
	~Shortcut() { resetIcons(); }
	
	void save(HANDLE hf);
	bool load(LPTSTR& rpszCurrent);
	
	void findExecutable(LPTSTR pszExecutable);
	void findSmallIconIndex();
	void findIcon();
	void resetIcons();
	
	void fillGetFileIcon(GETFILEICON* pgfi, bool bSmallIcon);
	
	int getSmallIconIndex();
	
	HICON getIcon()
	{
		if (!m_hIcon)
			findIcon();
		return m_hIcon;
	}
	
	void appendItemToString(String& rs) const;
	
	void onGetFileInfo(GETFILEICON& gfi);
	
	void getColumnText(int iColumn, String& rs) const;
	
	static int s_iSortColumn;
	static int CALLBACK compare(const Shortcut* psh1, const Shortcut* psh2, LPARAM lParamSort);
	
	void appendMenuItem(HMENU hMenu, UINT id) const;
	bool execute(bool bFromHotkey) const;
	
	bool match(BYTE vk, WORD vkFlags, const int aiCondState[], LPCTSTR pszProgram) const;
	bool testConflict(const Keystroke& ks, const String asProgram[], bool bProgramsOnly) const;
	
	String* getPrograms() const;
	void cleanPrograms();
	
	static bool tryChangeDirectory(HWND hdlg, LPCTSTR pszDirectory);
};


struct SPECIALKEY
{
	BYTE vk;
	int  vkFlags;
	int  tok;
};

const int nbSpecialKey = 4;
extern const SPECIALKEY e_aSpecialKey[nbSpecialKey];



bool AskKeystroke(HWND hwndParent, Shortcut* pksEdited, Keystroke& rksResult);


extern Shortcut *e_pshFirst;   // Shortcuts linked list


void shortcutsLoad();
void shortcutsSave();
void shortcutsClear();
void shortcutsCopyToClipboard();
