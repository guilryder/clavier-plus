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
#include "Global.h"

HANDLE e_hHeap;
HINSTANCE e_hInst;
HWND e_hwndInvisible;


static WNDPROC s_prcLabel;
static LRESULT CALLBACK prcWebLink(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static int CALLBACK prcBrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);


int e_lang;



int messageBox(HWND hWnd, UINT idString, UINT uType)
{
	TCHAR pszText[256];
	loadStringAuto(idString, pszText);
	return MessageBox(hWnd, pszText, pszApp, uType);
}


void getWindowSize(HWND hWnd, SIZE& size)
{
	RECT rc;
	GetWindowRect(hWnd, &rc);
	size.cx = rc.right  - rc.left;
	size.cy = rc.bottom - rc.top;
}


void initializeWebLink(HWND hDlg, UINT idControl, LPCTSTR pszLink)
{
	const HWND hwnd = GetDlgItem(hDlg, idControl);
	SetWindowLong(hwnd, GWL_USERDATA, (LONG)pszLink);
	s_prcLabel = SubclassWindow(hwnd, prcWebLink);
}


void writeFile(HANDLE hf, LPCTSTR psz)
{
	DWORD len;
	WriteFile(hf, psz, lstrlen(psz) * sizeof(TCHAR), &len, NULL);
}


// The link itself is stored in GWL_USERDATA style
LRESULT CALLBACK prcWebLink(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		
		case WM_SETCURSOR:
			SetCursor(LoadCursor(NULL, IDC_HAND));
			return TRUE;
		
		case WM_NCHITTEST:
			return HTCLIENT;
		
		case WM_LBUTTONDOWN:
			ShellExecute(hWnd, NULL, (LPCTSTR)GetWindowLong(hWnd, GWL_USERDATA), NULL, NULL, SW_SHOWDEFAULT);
			return 0;
	}
	
	return CallWindowProc(s_prcLabel, hWnd, uMsg, wParam, lParam);
}

int CALLBACK prcBrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM, LPARAM lpData)
{
	if (uMsg == BFFM_INITIALIZED)
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	return 0;
}


bool BrowseForFolder(HWND hwndParent, LPTSTR pszDirectory)
{
	TCHAR pszTitle[MAX_PATH];
	loadStringAuto(IDS_BROWSEDIR, pszTitle);
	
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(bi));
	bi.hwndOwner = hwndParent;
	bi.lpszTitle = pszTitle;
	bi.ulFlags   = BIF_RETURNONLYFSDIRS;
	bi.lpfn      = prcBrowseForFolderCallback;
	bi.lParam    = (LPARAM)pszDirectory;
	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if (!pidl || !SHGetPathFromIDList(pidl, pszDirectory))
		return false;
	LPMALLOC pMalloc;
	if (SUCCEEDED(SHGetMalloc(&pMalloc))) {
		pMalloc->Free(pidl);
		pMalloc->Release();
	}
	
	return true;
}



//------------------------------------------------------------------------
// TranslatedString
//------------------------------------------------------------------------

void TranslatedString::load(UINT id)
{
	loadStringAuto(id, m_apsz[e_lang]);
}


void setResourceLanguage(int lang)
{
	static const LANGID s_alcid[] =
	{
		MAKELANGID(LANG_FRENCH,  SUBLANG_FRENCH),
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		MAKELANGID(LANG_GERMAN,  SUBLANG_GERMAN),
	};
#ifdef _DEBUG
	if (nbArray(s_alcid) != langCount)
		*(int*)0 = 0;
#endif
	
	e_lang = lang;
	SetThreadLocale(s_alcid[lang]);
}



void findFullPath(LPTSTR pszPath, LPTSTR pszFullPath)
{
	PathUnquoteSpaces(pszPath);
	if ((DWORD)FindExecutable(pszPath, NULL,pszFullPath) <= 32 &&
	    !SearchPath(NULL, pszPath, NULL, nbArray(pszFullPath), pszFullPath, NULL))
		lstrcpy(pszFullPath, pszPath);
}


void shellExecuteCmdLine(LPCTSTR pszCommand, LPCTSTR pszDirectory, int nShow)
{
	// Split the command line: get the file and the arguments
	TCHAR pszPath[MAX_PATH];
	lstrcpyn(pszPath, pszCommand, nbArray(pszPath));
	PathRemoveArgs(pszPath);
	
	TCHAR pszDirectoryBuf[MAX_PATH];
	if (!pszDirectory || !*pszDirectory) {
		findFullPath(pszPath, pszDirectoryBuf);
		PathRemoveFileSpec(pszDirectoryBuf);
		pszDirectory = pszDirectoryBuf;
		PathQuoteSpaces(pszPath);
	}
	
	// Run the command line
	ShellExecute(e_hwndInvisible, NULL, pszPath,
		PathGetArgs(pszCommand), pszDirectory, nShow);
}


DWORD WINAPI threadShellExecute(void* pParam)
{
	THREAD_SHELLEXECUTE &data = *(THREAD_SHELLEXECUTE*)pParam;
	shellExecuteCmdLine(data.sCommand, data.sDirectory, data.nShow);
	delete (THREAD_SHELLEXECUTE*)pParam;
	return 0;
}
