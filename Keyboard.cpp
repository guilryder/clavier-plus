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


#include "StdAfx.h"
#include "App.h"


const SPECIALKEY e_aSpecialKey[nbSpecialKey] =
{
	{ VK_LWIN,    MOD_WIN,     tokWin },
	{ VK_CONTROL, MOD_CONTROL, tokCtrl },
	{ VK_SHIFT,   MOD_SHIFT,   tokShift },
	{ VK_MENU,    MOD_ALT,     tokAlt },
};



static Keystroke *s_pksEditedOld;    // Pointer to the old value of the currently edited keystroke
static bool       s_ksReset;         // TRUE if we should reset the keystroke textbox ASAP
static Keystroke  s_ks;              // Currently edited keystroke value

static WNDPROC s_prcKeystrokeCtl;

static LRESULT CALLBACK prcKeystrokeCtl(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);



// Shortcut keystroke and conditions dialog box
BOOL CALLBACK prcKeystroke(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
	switch (uMsg) {
		
		// Initialization
		case WM_INITDIALOG:
			{
				centerParent(hDlg);
				
				const HWND hctl = GetDlgItem(hDlg, IDCTXT);
				s_prcKeystrokeCtl = SubclassWindow(hctl, prcKeystrokeCtl);
				PostMessage(hctl, WM_KEYSTROKE, 0,0);
				
				// Load condition names
				TCHAR pszConds[bufString];
				loadStringAuto(IDS_CONDITIONS, pszConds);
				TCHAR *pc = pszConds;
				for (int iCond = 0; iCond < condCount; iCond++) {
					while (*pc != _T(';'))
						pc++;
					*pc++ = _T('\0');
				}
				
				for (int i = 0; i < condTypeCount; i++) {
					const HWND hcbo = GetDlgItem(hDlg, IDCCBO_CAPSLOCK + i);
					pc = pszConds;
					for (int j = 0; j < condCount; j++) {
						ComboBox_AddString(hcbo, pc);
						pc += lstrlen(pc) + 1;
					}
					ComboBox_SetCurSel(hcbo, s_ks.m_aCond[i]);
				}
			}
			return TRUE;
		
		// Command
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				
				case IDOK:
					{
						UINT idErrorString;
						
						if (!s_ks.m_vk) {
							idErrorString = ERR_INVALID_SHORTCUT;
							messageBox(hDlg, idErrorString, MB_ICONEXCLAMATION);
							SetFocus(GetDlgItem(hDlg, IDCTXT));
							break;
						}
						
						for (int i = 0; i < condTypeCount; i++)
							s_ks.m_aCond[i] = ComboBox_GetCurSel(GetDlgItem(hDlg, IDCCBO_CAPSLOCK + i));
					}
					// Fall-through
				
				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
					break;
			}
			break;
	}
	
	return FALSE;
}


// Edit text of keystroke dialog box
LRESULT CALLBACK prcKeystrokeCtl(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		
		case WM_LBUTTONDOWN:
			SetFocus(hWnd);
			break;
		
		case WM_SETCURSOR:
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			return TRUE;
		
		
		case WM_GETDLGCODE:
			return DLGC_WANTALLKEYS;
		
		case WM_CHAR:
		case WM_DEADCHAR:
		case WM_SYSCHAR:
		case WM_SYSDEADCHAR:
			return 0;
		
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (s_ksReset)
				s_ks.reset();
			// Fall-through
		
		case WM_KEYUP:
		case WM_SYSKEYUP:
			{
				// Flags
				bool bIsSpecial =
					(wParam >= VK_LSHIFT && wParam <= VK_RMENU) ||
					(wParam >= VK_LWIN   && wParam <= VK_RWIN);
				for (int i = 0; i < nbArray(e_aSpecialKey); i++) {
					if (wParam == e_aSpecialKey[i].vk)
						bIsSpecial = true;
					if (GetKeyState(e_aSpecialKey[i].vk) & 0x80)
						s_ks.m_vkFlags |= e_aSpecialKey[i].vkFlags;
					else if (!s_ks.m_vk)
						s_ks.m_vkFlags &= ~e_aSpecialKey[i].vkFlags;
				}
				
				// Key
				if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) {
					s_ksReset = false;
					if (!bIsSpecial)
						s_ks.m_vk = (BYTE)wParam;
				}else
					s_ksReset |= !bIsSpecial;
			}
			// Fall-through
		
		// Display keystroke name
		case WM_KEYSTROKE:
			{
				TCHAR pszHotKey[bufHotKey];
				s_ks.getKeyName(pszHotKey);
				if (!*pszHotKey)
					lstrcpy(pszHotKey, getToken(tokNone));
				
				SetWindowText(hWnd, pszHotKey);
				const int len = lstrlen(pszHotKey);
				Edit_SetSel(hWnd, len,len);
			}
			return 0;
	}
	
	// Ignore mouse events
	if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
		return 0;
	
	return CallWindowProc(s_prcKeystrokeCtl, hWnd, uMsg, wParam, lParam);
}


bool AskKeystroke(HWND hwndParent, Shortcut* pksEdited, Keystroke& rksResult)
{
	s_pksEditedOld = pksEdited;
	s_ksReset = true;
	
	s_ks = rksResult;
	VERIF(IDOK == dialogBox(IDD_KEYSTROKE, hwndParent, prcKeystroke));
	
	rksResult = s_ks;
	return true;
}



//------------------------------------------------------------------------
// Keystroke
//------------------------------------------------------------------------

// Get the human readable name of the keystroke
ATOM Keystroke::getKeyName(LPTSTR pszHotKey, bool bGetAtom) const
{
	*pszHotKey = _T('\0');
	
	// Special keys
	for (int i = 0; i < nbArray(e_aSpecialKey); i++) {
		if (m_vkFlags & e_aSpecialKey[i].vkFlags) {
			StrNCat(pszHotKey, getToken(e_aSpecialKey[i].tok), bufHotKey);
			StrNCat(pszHotKey, _T(" + "), bufHotKey);
		}
	}
	
	// Main key
	getKeyName(m_vk, pszHotKey);
	
	return (bGetAtom) ? GlobalAddAtom(pszHotKey) : (ATOM)0;
}


bool Keystroke::isKeyExtended(UINT vk)
{
	return vk == VK_NUMLOCK || vk == VK_DIVIDE || (vk >= VK_PRIOR && vk <= VK_DELETE);
}

// Get a key name and add it to pszHotKey
void Keystroke::getKeyName(UINT vk, LPTSTR pszHotKey)
{
	long lScan = (::MapVirtualKey(vk, 0)<<16) | (1L<<25);
	if (isKeyExtended(vk))
		lScan |= 1L<<24;
	
	const int len = lstrlen(pszHotKey);
	::GetKeyNameText(lScan, pszHotKey + len, bufHotKey - len);
	if (vk >= 0xA6 && vk <= 0xB7 || (vk && !*pszHotKey))
		wsprintf(pszHotKey + len, _T("#%d"), vk);
}


// Test for inclusion
bool Keystroke::match(BYTE vk, WORD vkFlags, const int aiCondState[]) const
{
	VERIF(m_vk == vk && m_vkFlags == vkFlags);
	for (int i = 0; i < condTypeCount; i++)
		VERIF(m_aCond[i] == condIgnore || m_aCond[i] == aiCondState[i]);
	return true;
}


// Register the hotkey
void Keystroke::registerHotKey()
{
	TCHAR pszHotKey[bufHotKey];
	RegisterHotKey(NULL, getKeyName(pszHotKey, true), m_vkFlags, m_vk);
}

// Unregister the hotkey
bool Keystroke::unregisterHotKey()
{
	TCHAR pszHotKey[bufHotKey];
	return ToBool(UnregisterHotKey(NULL, getKeyName(pszHotKey, true)));
}


// Serialize the shortcut itself (the key name)
void Keystroke::serialize(LPTSTR psz)
{
	bool bSkipPlus = false;
	
	for (;;) {
		
		// Skip spaces and '+'
		while (*psz == ' ' || (*psz == '+' && bSkipPlus)) {
			if (*psz == '+')
				bSkipPlus = false;
			psz++;
		}
		bSkipPlus = true;
		if (!*psz)
			break;
		
		// Get the next word
		TCHAR *pcNext = psz;
		while (*pcNext && *pcNext != ' ' && *pcNext != '+')
			pcNext++;
		const TCHAR cOldNext = *pcNext;
		*pcNext = 0;
		
		
		const int tok = findToken(psz);
		if (tokWin <= tok && tok <= tokAlt) {
			// Special key token
			
			for (int i = 0; i < nbArray(e_aSpecialKey); i++) {
				if (e_aSpecialKey[i].tok == tok) {
					m_vkFlags |= e_aSpecialKey[i].vkFlags;
					break;
				}
			}
			
		}else{
			// Normal key token
			
			*pcNext = cOldNext;
			
			for (int vk = VK_BACK; vk < 256; vk++) {
				TCHAR pszHotKey[bufHotKey] = _T("");
				getKeyName(vk, pszHotKey);
				
				if (!lstrcmpi(psz, pszHotKey)) {
					m_vk = (BYTE)vk;
					break;
				}
			}
			
			// Stop here
			break;
		}
		
		psz = pcNext;
		if (cOldNext)
			psz++;
	}
}


// Simulate the typing of the keystroke
void Keystroke::simulateTyping() const
{
	BYTE abKeys[256];
	ZeroMemory(abKeys, sizeof(abKeys));
	
	int i;
	
	// Reset the keyboard
	SetKeyboardState(abKeys);
	
	// Press the special keys
	for (i = 0; i < nbArray(e_aSpecialKey); i++)
		if (m_vkFlags & e_aSpecialKey[i].vkFlags)
			keybd_event(e_aSpecialKey[i].vk, 0, 0, 0);
		else
			keybd_event(e_aSpecialKey[i].vk, 0, KEYEVENTF_KEYUP, 0);
	
	// Press and release the main key
	const DWORD dwFlags = (isKeyExtended(m_vk)) ? KEYEVENTF_EXTENDEDKEY : 0;
	keybd_event(m_vk, 0, dwFlags, 0);
	keybd_event(m_vk, 0, dwFlags | KEYEVENTF_KEYUP, 0);
	
	// Release the special keys
	for (i = 0; i < nbArray(e_aSpecialKey); i++)
		if (m_vkFlags & e_aSpecialKey[i].vkFlags)
			keybd_event(e_aSpecialKey[i].vk, 0, KEYEVENTF_KEYUP, 0);
	
	// Reset the keyboard again
	SetKeyboardState(abKeys);
}


// Get the keyboard focus and the focus window
void Keystroke::catchKeyboardFocus(HWND& rhwndFocus)
{
	const DWORD idThreadTo = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
	const DWORD idThread   = GetCurrentThreadId();
	AttachThreadInput(idThread, idThreadTo, TRUE);
	rhwndFocus = GetFocus();
	AttachThreadInput(idThread, idThreadTo, FALSE);
}



//------------------------------------------------------------------------
// Settings loading and saving
//------------------------------------------------------------------------

int e_acxCol[colCount];
static const int s_acxColOrig[] = { 40, 20, 20 };

SIZE e_sizeMainDialog = { 0, 0 };
bool e_bMaximizeMainDialog = false;


// Get the INI file path
// pszIniFile should be a MAX_PATH length buffer
void iniGetPath(LPTSTR pszIniFile)
{
	GetModuleFileName(e_hInst, pszIniFile, MAX_PATH);
	CharLower(pszIniFile);
	PathRenameExtension(pszIniFile, _T(".ini"));
}


// Open the key for Clavier+ launching at Windows startup
HKEY openAutoStartKey(LPTSTR pszPath)
{
	if (!GetModuleFileName(NULL, pszPath, MAX_PATH))
		*pszPath = 0;
	
	HKEY hKey;
	if (ERROR_SUCCESS != RegOpenKey(HKEY_CURRENT_USER,
	    _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), &hKey))
		hKey = NULL;
	
	return hKey;
}


void iniLoad()
{
	e_bIconVisible = true;
	
	for (int i = 0; i < colCount; i++)
		e_acxCol[i] = s_acxColOrig[i];
	
	TCHAR pszIniFile[MAX_PATH];
	iniGetPath(pszIniFile);
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
	if (size == INVALID_FILE_SIZE)
		goto Error;
	
	BYTE *const pbBuffer = new BYTE[size + 2];
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
#endif
	}else{
#ifdef UNICODE
		LPSTR wsz = (LPSTR)pbBuffer;
		const int buf = lstrlenA(psz) + 1;
		pszCurrent = new TCHAR[buf];
		MultiByteToWideChar(CP_ACP, 0, wsz, -1, pszCurrent, buf);
#else
		pszCurrent = (LPTSTR)pbBuffer;
#endif
	}
	
	Keystroke ks;
	do{
		Shortcut *const psh = new Shortcut(ks);
		if (psh->load(pszCurrent)) {
			psh->m_pNext = e_pshFirst;
			e_pshFirst = psh;
			psh->registerHotKey();
		}else
			delete psh;
	}while (pszCurrent);
	
	delete [] pbBuffer;
}


void iniSave()
{
	TCHAR pszIniFile[MAX_PATH];
	iniGetPath(pszIniFile);
	const HANDLE hf = CreateFile(pszIniFile,
		GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hf == INVALID_HANDLE_VALUE) {
		messageBox(NULL, ERR_SAVING_INI);
		return;
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
	writeFile(hf, _T("\r\n"));
	
	for (Shortcut *psh = e_pshFirst; psh; psh = psh->m_pNext)
		psh->save(hf);
	
	CloseHandle(hf);
}



//------------------------------------------------------------------------
// Shortcut
//------------------------------------------------------------------------

// Test for inclusion
bool Shortcut::match(BYTE vk, WORD vkFlags, const int aiCondState[], LPCTSTR pszProgram) const
{
	VERIF(Keystroke::match(vk, vkFlags, aiCondState));
	if (m_sPrograms.isEmpty())
		return true;
	
	return pszProgram && containsProgram(pszProgram);
}



// Test for intersection
bool Shortcut::testConflict(const Keystroke& ks, const String asProgram[]) const
{
	VERIF(m_vk == ks.m_vk && m_vkFlags == ks.m_vkFlags);
	for (int i = 0; i < condTypeCount; i++)
		VERIF(m_aCond[i] == condIgnore || ks.m_aCond[i] == condIgnore || m_aCond[i] == ks.m_aCond[i]);
	
	if (!asProgram || m_sPrograms.isEmpty())
		return true;
	
	for (int i = 0; asProgram[i].isSome(); i++)
		if (containsProgram(asProgram[i]))
			return true;
	
	return false;
}


// Get programs array, NULL if no program.
String* Shortcut::getPrograms() const
{
	VERIFP(m_sPrograms.isSome(), NULL);
	
	const LPCTSTR pszPrograms = m_sPrograms;
	int nbProgram = 0;
	for (int i = 0; pszPrograms[i]; i++)
		if (pszPrograms[i] == _T(';') && i > 0 && pszPrograms[i - 1] != _T(';'))
			nbProgram++;
	
	String *const asProgram = new String[nbProgram + 2];
	nbProgram = 0;
	for (int i = 0; pszPrograms[i]; i++) {
		if (pszPrograms[i] == _T(';')) {
			if (i > 0 && pszPrograms[i - 1] != _T(';'))
				nbProgram++;
		}else
			asProgram[nbProgram] += pszPrograms[i];
	}
	
	return asProgram;
}

// Eliminate duplicates in programs
void Shortcut::cleanPrograms()
{
	String *const asProgram = getPrograms();
	m_sPrograms.empty();
	VERIFV(asProgram);
	
	for (int i = 0; asProgram[i].isSome(); i++) {
		for (int j = 0; j < i; j++)
			if (!lstrcmpi(asProgram[i], asProgram[j]))
				goto Next;
		if (m_sPrograms.isSome())
			m_sPrograms += _T(';');
		m_sPrograms += asProgram[i];
		
	Next:
		;
	}
	
	delete [] asProgram;
}




bool Shortcut::containsProgram(LPCTSTR pszProgram) const
{
	LPCTSTR pszPrograms = m_sPrograms;
	const int lenCompare        = lstrlen(pszProgram);
	const int lenProgramsBrowse = lstrlen(pszPrograms) - lenCompare;
	
	bool bCompare = true;
	for (int i = 0; i <= lenProgramsBrowse; i++) {
		if (pszPrograms[i] == _T(';'))
			bCompare = true;
		else if (bCompare) {
			if (CSTR_EQUAL == CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE,
			    pszPrograms + i, lenCompare, pszProgram, lenCompare))
				return true;
			bCompare = false;
		}
	}
	
	return false;
}


void copyShortcutsListToClipboard()
{
	if (!OpenClipboard(NULL))
		return;
	EmptyClipboard();
	
	// Get the text
	String s;
	for (const Shortcut *psh = e_pshFirst; psh; psh = psh->m_pNext)
		psh->appendItemToString(s);
	
	// Allocate and fill shared buffer
	const HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (s.getLength() + 1) * sizeof(TCHAR));
	LPTSTR psz = (LPTSTR)GlobalLock(hMem);
	lstrcpy(psz, s);
	GlobalUnlock(hMem);
	
	// Copy buffer to clipboard
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
}



bool Keystroke::askSendKeys(HWND hwndParent, Keystroke& rks)
{
	s_ks.reset();
	
	VERIF(IDOK == dialogBox(IDD_SENDKEYS, hwndParent, prcSendKeys));
	
	rks = s_ks;
	return true;
}


BOOL CALLBACK Keystroke::prcSendKeys(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
	switch (uMsg) {
		
		// Initialization
		case WM_INITDIALOG:
			{
				centerParent(hDlg);
				
				const HWND hctl = GetDlgItem(hDlg, IDCTXT);
				s_prcKeystrokeCtl = SubclassWindow(hctl, prcKeystrokeCtl);
				PostMessage(hctl, WM_KEYSTROKE, 0,0);
			}
			return TRUE;
		
		// Command
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
					break;
			}
			break;
	}
	
	return FALSE;
}
