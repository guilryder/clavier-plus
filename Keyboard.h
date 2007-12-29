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

#include "App.h"

#define IsKeyDown(vk) \
	(GetKeyState(vk) & 0x8000)

const BYTE bKeyDown = 0x80;

const int vkFlagsRightOffset = 16;


enum {
	condTypeShiftLock,
	condTypeNumLock,
	condTypeScrollLock,
	condTypeCount
};

enum {
	condIgnore,
	condYes,
	condNo,
	condCount
};


class Keystroke {
public:
	
	BYTE m_vk;
	DWORD m_vkFlags;
	int m_aCond[condTypeCount];
	bool m_bDistinguishLeftRight;
	
	
public:
	
	Keystroke() {
		resetAll();
	}
	
	void reset() {
		m_vk = 0;
		m_vkFlags = 0;
	}
	
	void resetAll() {
		reset();
		m_bDistinguishLeftRight = false;
		for (int i = 0; i < condTypeCount; i++) {
			m_aCond[i] = condIgnore;
		}
	}
	
	
	WORD getVkFlagsNoSide() const {
		return (WORD)m_vkFlags | (WORD)(m_vkFlags >> vkFlagsRightOffset);
	}
	
	void getKeyName(LPTSTR pszHotKey) const;
	
	void serialize(LPTSTR psz);
	void simulateTyping(HWND hwndFocus, bool bSpecialKeys = true) const;
	
	bool match(const Keystroke& ks) const;
	
	bool canReleaseSpecialKeys() const {
		return (0xA6 > m_vk || m_vk > 0xB7);
	}
	
	static bool askSendKeys(HWND hwnd_parent, Keystroke& rks);
	
	
	static void keybdEvent(UINT vk, bool bUp);
	static BYTE filterVK(BYTE vk) {
		return (vk == VK_CLEAR) ? VK_NUMPAD5 : vk;
	}
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

struct GETFILEICON {
	Shortcut* psh;
	SHFILEINFO shfi;
	TCHAR pszExecutable[MAX_PATH];
	UINT uFlags;
	bool bOK;
};


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
	
	
protected:
	
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


struct SPECIALKEY {
	BYTE vk;
	BYTE vkLeft;
	DWORD vkFlags;
	int tok;
};

const int nbSpecialKey = 4;
extern const SPECIALKEY e_aSpecialKey[nbSpecialKey];


bool askKeystroke(HWND hwnd_parent, Shortcut* pksEdited, Keystroke& rksResult);


extern Shortcut* e_pshFirst;  // Shortcuts linked list


void shortcutsLoad();
void shortcutsMerge(LPCTSTR pszIniFile);
void shortcutsSave();
void shortcutsClear();
void shortcutsCopyToClipboard(const String& rs);
