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


#include "StdAfx.h"
#include "Shortcut.h"

namespace shortcut {

static Shortcut* s_first_shortcut;

CRITICAL_SECTION GuardList::s_critical_section;

const int aiShowOption[nbShowOption] = {
	SW_NORMAL, SW_MINIMIZE, SW_MAXIMIZE,
};

static const int s_acxColOrig[] = { 40, 20, 20 };


enum MOUSEMOVE_TYPE {
	mouseMoveAbsolute,
	mouseMoveRelativeToWindow,
	mouseMoveRelativeToCurrent,
};

static void commandEmpty(DWORD& ridThread, HWND& rhwndFocus);
static void commandWait(LPTSTR pszArg);
static bool commandFocus(DWORD& ridThread, HWND& rhwndFocus, LPTSTR pszArg);
static void commandCopy(LPTSTR pszArg);
static void commandMouseButton(LPTSTR pszArg);
static void commandMouseMove(POINT ptOrigin, LPTSTR pszArg);
static void commandMouseWheel(LPTSTR pszArg);


const LPCTSTR pszLineSeparator = _T("-\r\n");


void initialize() {
	s_first_shortcut = NULL;
	InitializeCriticalSection(&GuardList::s_critical_section);
}

void terminate() {
	DeleteCriticalSection(&GuardList::s_critical_section);
}

Shortcut* getFirst() {
	return s_first_shortcut;
}


Shortcut::Shortcut(const Shortcut& sh) : Keystroke(sh) {
	m_next_shortcut = NULL;
	m_bCommand = sh.m_bCommand;
	m_nShow = sh.m_nShow;
	m_bSupportFileOpen = sh.m_bSupportFileOpen;
	m_bProgramsOnly = sh.m_bProgramsOnly;
	m_iSmallIcon = sh.m_iSmallIcon;
	m_hIcon = CopyIcon(sh.m_hIcon);
	
	m_sDescription = sh.m_sDescription;
	m_sText = sh.m_sText;
	m_sCommand = sh.m_sCommand;
	m_sDirectory = sh.m_sDirectory;
	m_sPrograms = sh.m_sPrograms;
}

Shortcut::Shortcut(const Keystroke& ks) : Keystroke(ks) {
	m_next_shortcut = NULL;
	m_bCommand = false;
	m_nShow = SW_NORMAL;
	m_bSupportFileOpen = false;
	m_bProgramsOnly = false;
	m_iSmallIcon = iconNeeded;
	m_hIcon = NULL;
}

void Shortcut::addToList() {
	m_next_shortcut = s_first_shortcut;
	s_first_shortcut = this;
}


//------------------------------------------------------------------------
// Serialization
//------------------------------------------------------------------------

void Shortcut::save(HANDLE hf) {
	cleanPrograms();
	
	TCHAR pszHotKey[bufHotKey], pszCode[bufCode];
	getKeyName(pszHotKey);
	wsprintf(pszCode, _T("%lu"), ((DWORD)m_vk) | (((DWORD)m_vkFlags) << 8));
	
	struct LINE {
		int tokKey;
		LPCTSTR pszValue;
	};
	LINE aLine[3 + 4 + 2 + condTypeCount];
	aLine[0].tokKey = tokShortcut;
	aLine[0].pszValue = pszHotKey;
	aLine[1].tokKey = tokCode;
	aLine[1].pszValue = pszCode;
	int nbLine = 2;
	
	if (m_bDistinguishLeftRight) {
		aLine[nbLine].tokKey = tokDistinguishLeftRight;
		aLine[nbLine].pszValue = _T("1");
		nbLine++;
	}
	
	for (int i = 0; i < condTypeCount; i++) {
		if (m_aCond[i] != condIgnore) {
			aLine[nbLine].tokKey = tokConditionCapsLock + i;
			aLine[nbLine].pszValue = getToken(m_aCond[i] - 1 + tokConditionYes);
			nbLine++;
		}
	}
	
	String sText;
	
	if (m_bCommand) {
		// Command
		
		aLine[nbLine].tokKey = tokCommand;
		aLine[nbLine].pszValue = m_sCommand;
		nbLine++;
		
		if (m_sDirectory.isSome()) {
			aLine[nbLine].tokKey = tokDirectory;
			aLine[nbLine].pszValue = m_sDirectory;
			nbLine++;
		}
		
		aLine[nbLine].tokKey = tokWindow;
		aLine[nbLine].pszValue = _T("");
		for (int i = 0; i < shortcut::nbShowOption; i++) {
			if (m_nShow == shortcut::aiShowOption[i]) {
				aLine[nbLine].pszValue = getToken(tokShowNormal + i);
				break;
			}
		}
		nbLine++;
		
		if (m_bSupportFileOpen) {
			aLine[nbLine].tokKey = tokSupportFileOpen;
			aLine[nbLine].pszValue = _T("1");
			nbLine++;
		}
		
		
	} else {
		// Text
		
		// Copy the text, replace "\r\n" by "\r\n>"
		// to handle multiple lines text
		for (const TCHAR *pcFrom = m_sText; *pcFrom; pcFrom++) {
			sText += *pcFrom;
			if (pcFrom[0] == _T('\r') && pcFrom[1] == _T('\n')) {
				sText += _T("\n>");
				pcFrom++;
			}
		}
		
		aLine[nbLine].tokKey = tokText;
		aLine[nbLine].pszValue = sText;
		nbLine++;
	}
	
	if (m_sPrograms.isSome()) {
		aLine[nbLine].tokKey = (m_bProgramsOnly) ? tokPrograms : tokAllProgramsBut;
		aLine[nbLine].pszValue = m_sPrograms;
		nbLine++;
	}
	
	if (m_sDescription.isSome()) {
		aLine[nbLine].tokKey = tokDescription;
		aLine[nbLine].pszValue = m_sDescription;
		nbLine++;
	}
	
	
	// Write all
	for (int i = 0; i < nbLine; i++) {
		writeFile(hf, getToken(aLine[i].tokKey));
		writeFile(hf, _T("="));
		writeFile(hf, aLine[i].pszValue);
		writeFile(hf, _T("\r\n"));
	}
	writeFile(hf, pszLineSeparator);
}

bool Shortcut::load(LPTSTR& rpszCurrent) {
	// Read
	
	m_vk = 0;
	m_vkFlags = 0;
	for (;;) {
		
		// Read the line
		LPTSTR pszLine = rpszCurrent;
		if (!pszLine) {
			break;
		}
		LPTSTR pszLineEnd = pszLine;
		while (*pszLineEnd && *pszLineEnd != _T('\n') && *pszLineEnd != _T('\r')) {
			pszLineEnd++;
		}
		if (*pszLineEnd) {
			*pszLineEnd = _T('\0');
			rpszCurrent = pszLineEnd + 1;
		} else {
			rpszCurrent = NULL;
		}
		
		int len = (int)(pszLineEnd - pszLine);
		
		// If end of shortcut, stop
		if (*pszLine == pszLineSeparator[0]) {
			break;
		}
		
		// If the line is empty, ignore it
		if (!len) {
			continue;
		}
		
		// If next line of text, get it
		if (*pszLine == _T('>') && !m_bCommand && *m_sText) {
			m_sText += _T("\r\n");
			m_sText += pszLine + 1;
			continue;
		}
		
		// Get the key name
		TCHAR *pcSep = pszLine;
		while (*pcSep && *pcSep != _T(' ') && *pcSep != _T('=')) {
			pcSep++;
		}
		const TCHAR cOldSel = *pcSep;
		*pcSep = 0;
		
		// Identify the key
		const int tokKey = findToken(pszLine);
		if (tokKey == tokNotFound) {
			continue;
		}
		
		// Get the value
		*pcSep = cOldSel;
		while (*pcSep && *pcSep != _T('=')) {
			pcSep++;
		}
		if (*pcSep) {
			pcSep++;
		}
		len -= (pcSep - pszLine);
		switch (tokKey) {
			
			// Language
			case tokLanguage:
				for (int lang = 0; lang < i18n::langCount; lang++) {
					if (!lstrcmpi(pcSep, getLanguageName(lang))) {
						i18n::setLanguage(lang);
					}
				}
				break;
			
			// Main window size
			case tokSize:
				while (*pcSep == _T(' ')) {
					pcSep++;
				}
				e_sizeMainDialog.cx = StrToInt(pcSep);
				
				skipUntilComma(pcSep);
				e_sizeMainDialog.cy = StrToInt(pcSep);
				
				skipUntilComma(pcSep);
				e_bMaximizeMainDialog = ToBool(StrToInt(pcSep));
				
				skipUntilComma(pcSep);
				e_bIconVisible = !StrToInt(pcSep);
				break;
			
			// Main window columns width
			case tokColumns:
				for (int i = 0; i < colCount - 1; i++) {
					if (!*pcSep) {
						break;
					}
					int cx = StrToInt(pcSep);
					if (cx >= 0) {
						e_acxCol[i] = cx;
					}
					skipUntilComma(pcSep);
				}
				break;
			
			// Sorting column
			case tokSorting:
				s_iSortColumn = StrToInt(pcSep);
				if (s_iSortColumn < 0 || s_iSortColumn >= colCount) {
					s_iSortColumn = colContents;
				}
				break;
			
			// Shortcut
			case tokShortcut:
				Keystroke::serialize(pcSep);
				break;
			
			// Code
			case tokCode:
				if (!m_vk) {
					const DWORD dwCode = (DWORD)StrToInt(pcSep);
					if (dwCode) {
						m_vk = filterVK(LOBYTE(dwCode));
						m_vkFlags = HIBYTE(dwCode);
					}
				}
				break;
			
			// Distinguish left/right
			case tokDistinguishLeftRight:
				m_bDistinguishLeftRight = ToBool(StrToInt(pcSep));
				break;
			
			// Description
			case tokDescription:
				m_sDescription = pcSep;
				break;
			
			// Text or command
			case tokText:
			case tokCommand:
				m_bCommand = (tokKey == tokCommand);
				((m_bCommand) ? m_sCommand : m_sText) = pcSep;
				break;
			
			// Directory
			case tokDirectory:
				m_sDirectory = pcSep;
				break;
			
			// Programs
			case tokPrograms:
			case tokAllProgramsBut:
				m_sPrograms = pcSep;
				m_bProgramsOnly = (tokKey == tokPrograms);
				cleanPrograms();
				break;
			
			// Window
			case tokWindow:
				{
					const int iShow = findToken(pcSep) - tokShowNormal;
					if (0 <= iShow && iShow <= shortcut::nbShowOption) {
						m_nShow = shortcut::aiShowOption[iShow];
					}
				}
				break;
			
			// Folder
			case tokSupportFileOpen:
				m_bSupportFileOpen = ToBool(StrToInt(pcSep));
				break;
			
			// Condition
			case tokConditionCapsLock:
			case tokConditionNumLock:
			case tokConditionScrollLock:
				{
					int cond = findToken(pcSep);
					if (tokConditionYes <= cond && cond <= tokConditionNo) {
						m_aCond[tokKey - tokConditionCapsLock] = cond - tokConditionYes + condYes;
					}
				}
				break;
		}
	}
	
	// Valid shortcut
	return ToBool(m_vk);
}


//------------------------------------------------------------------------
// Executing
//------------------------------------------------------------------------

bool Shortcut::execute(bool bFromHotkey) const {
	BYTE abKeyboard[256], abKeyboardNew[256];
	GetKeyboardState(abKeyboard);
	
	// Typing simulation requires the application has the keyboard focus
	HWND hwndFocus;
	DWORD idThread;
	Keystroke::catchKeyboardFocus(hwndFocus, idThread);
	memcpy(abKeyboardNew, abKeyboard, sizeof(abKeyboard));
	
	const bool bCanReleaseSpecialKeys = bFromHotkey && canReleaseSpecialKeys();
	
	if (bCanReleaseSpecialKeys) {
		
		// Simulate special keys release, to avoid side effects
		// Avoid to leave Alt and Shift down, alone, at the same time
		// because it is one of Windows keyboard layout shortcut:
		// simulate Ctrl down, release Alt and Shift, then release Ctrl
		bool bReleaseControl = false;
		if ((abKeyboard[VK_MENU] & keyDownMask) &&
				(abKeyboard[VK_SHIFT] & keyDownMask) &&
				!(abKeyboard[VK_CONTROL] & keyDownMask)) {
			bReleaseControl = true;
			keybdEvent(VK_CONTROL, false);
			abKeyboardNew[VK_CONTROL] = keyDownMask;
		}
		
		for (int i = 0; i < nbArray(e_aSpecialKey); i++) {
			abKeyboardNew[e_aSpecialKey[i].vk] = 0;
		}
		abKeyboardNew[m_vk] = 0;
		SetKeyboardState(abKeyboardNew);
		
		if (bReleaseControl) {
			keybdEvent(VK_CONTROL, true);
		}
	}
	
	if (m_bCommand) {
		// Execute a command line
		
		// Required because the launched program
		// can be a script that simulates keystrokes
		if (bCanReleaseSpecialKeys) {
			releaseSpecialKeys(abKeyboard);
		}
		
		if (!m_bSupportFileOpen || !tryChangeDirectory(hwndFocus, m_sCommand)) {
			clipboardToEnvironment();
			THREAD_SHELLEXECUTE *const pParams = new THREAD_SHELLEXECUTE(
				m_sCommand, m_sDirectory, m_nShow);
			startThread(threadShellExecute, pParams);
		}
		
	} else {
		// Text
		
		enum KIND {
			kindNone,
			kindText,
			kindKeystroke,
		};
		KIND lastKind = kindNone;
		
		// Send the text to the window
		bool bBackslash = false;
		LPCTSTR pszText = m_sText;
		for (int i = 0; pszText[i]; i++) {
			const UTCHAR c = (UTCHAR)pszText[i];
			if (c == _T('\n')) {
				continue;
			}
			
			if (!bBackslash && c == _T('\\')) {
				bBackslash = true;
				continue;
			}
			
			if (bBackslash || c != _T('[')) {
				if (lastKind != kindText) {
					sleepBackground(0);
				}
				lastKind = kindText;
				
				bBackslash = false;
				const WORD vkMask = VkKeyScan(c);
				PostMessage(hwndFocus, WM_CHAR, c, MAKELPARAM(1,
					MapVirtualKey(LOBYTE(vkMask), 0)));
				
			} else {
				// Inline shortcut, or inline command line execution
				
				// Go to the end of the shortcut, and serialize it
				const LPCTSTR pcStart = pszText + i + 1;
				const TCHAR *pcEnd = pcStart;
				while (*pcEnd && (pcEnd[-1] == _T('\\') || pcEnd[0] != _T(']'))) {
					pcEnd++;
				}
				if (!*pcEnd) {
					break;
				}
				
				String sInside;
				bool bBackslash2 = false;
				for (const TCHAR *pc = pcStart; pc < pcEnd; pc++) {
					if (bBackslash2) {
						bBackslash2 = false;
						sInside += *pc;
					} else if (*pc == _T('\\')) {
						bBackslash2 = true;
					} else {
						sInside += *pc;
					}
				}
				
				if (sInside.isEmpty()) {
					// Nothing: catch the focus
					
					lastKind = kindNone;
					commandEmpty(idThread, hwndFocus);
					
				} else if (*pcStart == _T('[') && *pcEnd == _T(']')) {
					// Double brackets: [[command line]]
					
					pcEnd++;
					
					// Required because the launched program
					// can be a script that simulates keystrokes
					releaseSpecialKeys(abKeyboard);
					
					clipboardToEnvironment();
					shellExecuteCmdLine((LPCTSTR)sInside + 1, NULL, SW_SHOWDEFAULT);
					
				} else if (*pcStart == _T('{') && pcEnd[-1] == _T('}')) {
					// Braces: [{command}]
					
					sInside[sInside.getLength() - 1] = _T('\0');
					const LPCTSTR pszCommand = sInside + 1;
					
					LPTSTR pszArg;
					for (pszArg = (LPTSTR)pszCommand; *pszArg && *pszArg != _T(','); pszArg++) {
						;
					}
					*pszArg++ = _T('\0');
					
					if (!lstrcmpi(pszCommand, _T("Wait"))) {
						commandWait(pszArg);
					} else if (!lstrcmpi(pszCommand, _T("Focus"))) {
						if (!commandFocus(idThread, hwndFocus, pszArg)) {
							break;
						}
					} else if (!lstrcmpi(pszCommand, _T("Copy"))) {
						commandCopy(pszArg);
					} else if (!lstrcmpi(pszCommand, _T("MouseButton"))) {
						commandMouseButton(pszArg);
					} else if (!lstrcmpi(pszCommand, _T("MouseMoveTo"))) {
						const POINT ptOrigin = { 0, 0 };
						commandMouseMove(ptOrigin, pszArg);
					} else if (!lstrcmpi(pszCommand, _T("MouseMoveToFocus"))) {
						RECT rcFocusedWindow;
						const HWND hwndOwner = GetAncestor(hwndFocus, GA_ROOT);
						if (!hwndOwner || !GetWindowRect(hwndOwner, &rcFocusedWindow)) {
							rcFocusedWindow.left = rcFocusedWindow.top = 0;
						}
						commandMouseMove((const POINT&)rcFocusedWindow, pszArg);
					} else if (!lstrcmpi(pszCommand, _T("MouseMoveBy"))) {
						POINT ptOrigin;
						GetCursorPos(&ptOrigin);
						commandMouseMove(ptOrigin, pszArg);
					} else if (!lstrcmpi(pszCommand, _T("MouseWheel"))) {
						commandMouseWheel(pszArg);
					}
				} else {
					// Simple brackets: [keystroke]
					
					releaseSpecialKeys(abKeyboard);
					
					lastKind = kindKeystroke;
					Keystroke ks;
					
					if (*pcStart == _T('|') && pcEnd[-1] == _T('|')) {
						// Brackets and pipe: [|characters as keystroke|]
						
						sInside[sInside.getLength() - 1] = _T('\0');
						for (LPCTSTR psz = (LPCTSTR)sInside; *++psz;) {
							const WORD wKey = VkKeyScan(*psz);
							if (wKey == (WORD)-1) {
								TCHAR pszCode[5];
								wsprintf(pszCode, _T("0%u"), (UTCHAR)*psz);
								
								const BYTE scanCodeAlt = (BYTE)MapVirtualKey(VK_MENU, 0);
								keybd_event(VK_MENU, scanCodeAlt, 0, 0);
								
								Keystroke ksDigit;
								ks.m_vkFlags = MOD_ALT;
								for (size_t i = 0; pszCode[i]; i++) {
									ks.m_vk = VK_NUMPAD0 + (BYTE)(pszCode[i] - _T('0'));
									ks.simulateTyping(hwndFocus, false);
								}
								
								keybd_event(VK_MENU, scanCodeAlt, KEYEVENTF_KEYUP, 0);
								
							} else {
								const BYTE bFlags = HIBYTE(wKey);
								ks.m_vk = LOBYTE(wKey);
								ks.m_vkFlags = 0;
								if (bFlags & (1 << 0)) {
									ks.m_vkFlags |= MOD_SHIFT;
								}
								if (bFlags & (1 << 1)) {
									ks.m_vkFlags |= MOD_CONTROL;
								}
								if (bFlags & (1 << 2)) {
									ks.m_vkFlags |= MOD_ALT;
								}
								
								ks.simulateTyping(hwndFocus);
							}
						}
						
					} else {
						// Simple brackets: [keystroke]
						
						ks.serialize(sInside.get());
						ks.simulateTyping(hwndFocus);
					}
				}
				
				i = pcEnd - pszText;
			}
		}
	}
	
	return !m_bCommand;
}


// []
// Sleep for 100 milliseconds and catch the focus.
// This is a shortcut for [{Focus,100}]
void commandEmpty(DWORD& ridThread, HWND& rhwndFocus) {
	sleepBackground(100);
	Keystroke::detachKeyboardFocus(ridThread);
	Keystroke::catchKeyboardFocus(rhwndFocus, ridThread);
}


// [{Wait,duration}]
// Sleep for a given number of milliseconds.
void commandWait(LPTSTR pszArg) {
	sleepBackground(StrToInt(pszArg));
}


// [{Focus,delay,[!]window_name}]
// Sleep for delay milliseconds and catch the focus.
// If window_name does not begin with '!', return false if the window is not found.
bool commandFocus(DWORD& ridThread, HWND& rhwndFocus, LPTSTR pszArg) {
	// Apply the delay
	const int delay = StrToInt(pszArg);
	skipUntilComma(pszArg);
	sleepBackground(delay);
	
	// Find the given window
	const bool bIgnoreNotFound = (pszArg[0] == _T('!'));
	if (bIgnoreNotFound) {
		pszArg++;
	}
	const LPCTSTR pszWindowName = pszArg;
	skipUntilComma(pszArg, true);
	
	if (*pszWindowName) {
		const HWND hwndTarget = findWindowByName(pszWindowName);
		if (hwndTarget) {
			SetForegroundWindow(hwndTarget);
		} else if (!bIgnoreNotFound) {
			return false;
		}
	}
	
	Keystroke::detachKeyboardFocus(ridThread);
	Keystroke::catchKeyboardFocus(rhwndFocus, ridThread);
	return true;
}


// [{Copy,text}]
// Copy the text argument to the clipboard.
void commandCopy(LPTSTR pszArg) {
	const HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE,
		(lstrlen(pszArg) + 1) * sizeof(*pszArg));
	VERIFV(hMem);
	
	const LPTSTR pszMem = (LPTSTR)GlobalLock(hMem);
	if (pszMem) {
		lstrcpy(pszMem, pszArg);
	}
	GlobalUnlock(hMem);
	
	OpenClipboard(NULL);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
}


// [{Mouse,state}] where state is 2 letters:
// 1 letter for button: L (left), M (middle), R (right)
// 1 letter for state: U (up), D (down)
// Simulate mouse clicks.
void commandMouseButton(LPTSTR pszArg) {
	CharUpper(pszArg);
	DWORD dwFlags = 0;
	switch (lstrlen(pszArg)) {
		case 2:
			switch (*(const WORD*)pszArg) {
				case 'DL':  dwFlags = MOUSEEVENTF_LEFTDOWN;  break;
				case 'UL':  dwFlags = MOUSEEVENTF_LEFTUP;  break;
				case 'DM':  dwFlags = MOUSEEVENTF_MIDDLEDOWN;  break;
				case 'UM':  dwFlags = MOUSEEVENTF_MIDDLEUP;  break;
				case 'DR':  dwFlags = MOUSEEVENTF_RIGHTDOWN;  break;
				case 'UR':  dwFlags = MOUSEEVENTF_RIGHTUP;  break;
			}
			if (dwFlags) {
				mouse_event(dwFlags, 0, 0, 0, 0);
				sleepBackground(0);
			}
			break;
		
		case 1:
			switch (*pszArg) {
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


// [{MouseMoveTo,x,y}], [{MouseMoveToFocus,x,y}], [{MouseMoveBy,dx,dy}]
// Move the mouse cursor.
void commandMouseMove(POINT ptOrigin, LPTSTR pszArg) {
	ptOrigin.x += StrToInt(pszArg);
	skipUntilComma(pszArg);
	ptOrigin.y += StrToInt(pszArg);
	SetCursorPos(ptOrigin.x, ptOrigin.y);
	sleepBackground(0);
}


// [{MouseWheel,offset}]
// Simulate a mouse wheel scroll.
void commandMouseWheel(LPTSTR pszArg) {
	const int offset = -StrToInt(pszArg) * WHEEL_DELTA;
	if (offset) {
		mouse_event(MOUSEEVENTF_WHEEL, 0, 0, (DWORD)offset, 0);
		sleepBackground(0);
	}
}


//------------------------------------------------------------------------
// Matching
//------------------------------------------------------------------------

// Get programs array, NULL if no program.
String* Shortcut::getPrograms() const {
	VERIFP(m_sPrograms.isSome(), NULL);
	
	const LPCTSTR pszPrograms = m_sPrograms;
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

// Eliminate duplicates in programs
void Shortcut::cleanPrograms() {
	String *const asProgram = getPrograms();
	m_sPrograms.empty();
	VERIFV(asProgram);
	
	for (int i = 0; asProgram[i].isSome(); i++) {
		for (int j = 0; j < i; j++) {
			if (!lstrcmpi(asProgram[i], asProgram[j])) {
				goto Next;
			}
		}
		if (m_sPrograms.isSome()) {
			m_sPrograms += _T(';');
		}
		m_sPrograms += asProgram[i];
		
	Next:
		;
	}
	
	delete [] asProgram;
}


bool Shortcut::containsProgram(LPCTSTR pszProgram) const {
	LPCTSTR pszPrograms = m_sPrograms;
	VERIF(*pszPrograms);
	
	const TCHAR *pcStart = pszPrograms;
	for (;;) {
		const TCHAR *pc = pcStart;
		while (*pc && *pc != _T(';')) {
			pc++;
		}
		
		if (CSTR_EQUAL == CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE,
				pcStart, pc - pcStart, pszProgram, -1)) {
			return true;
		}
		
		if (!*pc) {
			return false;
		}
		pcStart = pc + 1;
	}
}

int Shortcut::computeMatchingLevel(const Keystroke& ks, LPCTSTR pszProgram) const {
	if (Keystroke::match(ks)) {
		if (pszProgram) {
			if ((m_sPrograms.isSome() && containsProgram(pszProgram)) ^ (!m_bProgramsOnly)) {
				return m_bProgramsOnly ? 2 : 1;
			}
		} else {
			// No program in the environment: always match.
			return 1;
		}
	}
	return 0;
}

MATCHING_RESULT e_matching_result;

void computeAllMatchingLevels(const Keystroke& ks, LPCTSTR pszProgram) {
	e_matching_result.matching_shortcuts_count = 0;
	e_matching_result.max_matching_level = 0;
	
	for (Shortcut* shortcut = getFirst(); shortcut; shortcut = shortcut->getNext()) {
		const int matching_level = shortcut->computeMatchingLevel(ks, pszProgram);
		shortcut->setMatchingLevel(matching_level);
		if (matching_level == e_matching_result.max_matching_level) {
			e_matching_result.matching_shortcuts_count++;
		} else if (matching_level > e_matching_result.max_matching_level) {
			e_matching_result.max_matching_level = matching_level;
			e_matching_result.matching_shortcuts_count = 1;
		}
	}
	
	if (e_matching_result.max_matching_level <= 0) {
		e_matching_result.matching_shortcuts_count = 0;
	}
}

Shortcut* getNextMatching(Shortcut* from_this, int min_matching_level) {
	while (from_this) {
		if (from_this->getMatchingLevel() >= min_matching_level) {
			return from_this;
		}
		from_this = from_this->getNext();
	}
	return NULL;
}


//------------------------------------------------------------------------
// Serialization
//------------------------------------------------------------------------

void loadShortcuts() {
	clearShortcuts();
	mergeShortcuts(e_pszIniFile);
}

void mergeShortcuts(LPCTSTR pszIniFile) {
	e_bIconVisible = true;
	
	memcpy(e_acxCol, s_acxColOrig, sizeof(s_acxColOrig));
	
	const HANDLE hf = CreateFile(pszIniFile,
		GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hf == INVALID_HANDLE_VALUE) {
		if (GetLastError() != ERROR_FILE_NOT_FOUND) {
Error:
			messageBox(NULL, ERR_LOADING_INI);
		}
		return;
	}
	
	DWORD size = GetFileSize(hf, NULL);
	if (size == INVALID_FILE_SIZE) {
		goto Error;
	}
	
	BYTE *pbBuffer = new BYTE[size + 2];
	DWORD lenRead;
	const bool bOK = ReadFile(hf, pbBuffer, size, &lenRead, NULL) && lenRead == size;
	CloseHandle(hf);
	
	if (!bOK) {
		delete [] pbBuffer;
		goto Error;
	}
	
	pbBuffer[size] = pbBuffer[size + 1] = 0;
	LPTSTR pszCurrent;
	if (IsTextUnicode(pbBuffer, size, NULL)) {
#ifdef UNICODE
		pszCurrent = (LPTSTR)pbBuffer;
#else
		LPWSTR wsz = (LPWSTR)pbBuffer;
		const int buf = lstrlenW(wsz) + 1;
		pszCurrent = new TCHAR[buf];
		WideCharToMultiByte(CP_ACP, 0, wsz, -1, pszCurrent, buf, NULL, NULL);
		pbBuffer = (BYTE*)pszCurrent;
#endif
	} else {
#ifdef UNICODE
		LPSTR asz = (LPSTR)pbBuffer;
		const int buf = lstrlenA(asz) + 1;
		pszCurrent = new TCHAR[buf];
		MultiByteToWideChar(CP_ACP, 0, asz, -1, pszCurrent, buf);
		pbBuffer = (BYTE*)pszCurrent;
#else
		pszCurrent = (LPTSTR)pbBuffer;
#endif
	}
	
	Keystroke ks;
	do {
		Shortcut *const psh = new Shortcut(ks);
		if (psh->load(pszCurrent)) {
			psh->addToList();
		} else {
			delete psh;
		}
	} while (pszCurrent);
	
	delete [] pbBuffer;
	HeapCompact(e_hHeap, 0);
}


void saveShortcuts() {
	HANDLE hf;
	for (;;) {
		hf = CreateFile(e_pszIniFile,
			GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		if (hf != INVALID_HANDLE_VALUE) {
			break;
		}
		VERIFV(messageBox(NULL, ERR_SAVING_INI, MB_ICONERROR | MB_RETRYCANCEL) == IDRETRY);
	}
	
	TCHAR psz[1024];
	
	wsprintf(psz, _T("%s=%s\r\n"),
		getToken(tokLanguage), getToken(tokLanguageName));
	writeFile(hf, psz);
	
	wsprintf(psz, _T("%s=%d,%d,%d,%d\r\n"),
		getToken(tokSize), e_sizeMainDialog.cx, e_sizeMainDialog.cy,
		e_bMaximizeMainDialog, !e_bIconVisible);
	writeFile(hf, psz);
	
	writeFile(hf, getToken(tokColumns));
	for (int i = 0; i < nbColSize; i++) {
		wsprintf(psz, (i == 0) ? _T("=%d") : _T(",%d"), e_acxCol[i]);
		writeFile(hf, psz);
	}
	
	wsprintf(psz, _T("\r\n%s=%d\r\n\r\n"),
		getToken(tokSorting), Shortcut::s_iSortColumn);
	writeFile(hf, psz);
	
	for (Shortcut* shortcut = getFirst(); shortcut; shortcut = shortcut->getNext()) {
		shortcut->save(hf);
	}
	
	CloseHandle(hf);
}


void clearShortcuts() {
	while (s_first_shortcut) {
		Shortcut *const old_first_shortcut = s_first_shortcut;
		s_first_shortcut = old_first_shortcut->getNext();
		delete old_first_shortcut;
	}
	s_first_shortcut = NULL;
}


void copyShortcutsToClipboard(const String& rs) {
	if (!OpenClipboard(NULL)) {
		return;
	}
	EmptyClipboard();
	
	// Allocate and fill shared buffer
	const HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (rs.getLength() + 1) * sizeof(TCHAR));
	LPTSTR psz = (LPTSTR)GlobalLock(hMem);
	lstrcpy(psz, rs);
	GlobalUnlock(hMem);
	
	// Copy buffer to clipboard
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
}

}  // shortcut namespace
