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

#include "Resource.h"


#ifdef UNICODE
typedef WCHAR UTCHAR;
#else
typedef unsigned char UTCHAR;
#endif


const LPCTSTR pszApp = _T("Clavier+");


extern HANDLE e_hHeap;
extern HINSTANCE e_hInst;
extern HWND e_hwndInvisible;    // Invisible background window


int messageBox(HWND hWnd, UINT idString, UINT uType = MB_ICONERROR);

void getWindowSize(HWND hWnd, SIZE& size);

void initializeWebLink(HWND hDlg, UINT idControl, LPCTSTR pszLink);

bool BrowseForFolder(HWND hwndParent, LPTSTR pszDirectory);

void writeFile(HANDLE hf, LPCTSTR psz);

LPCTSTR getToken(int tok);
int findToken(LPCTSTR pszToken);



const size_t bufString = 128;
const size_t bufHotKey = 128;
const size_t bufCode   =  32;


enum
{
	langFR,
	langUS,
	langDE,
	langCount
};

extern int e_lang;
void setResourceLanguage(int lang);


class TranslatedString
{
public:
	
	void load(UINT id);
	
	void set(LPCTSTR psz)
	{
		lstrcpy(m_apsz[e_lang], psz);
	}
	
	LPCTSTR get(int lang) const { return m_apsz[lang]; }
	LPCTSTR get() const { return get(e_lang); }
	
	
private:
	
	TCHAR m_apsz[langCount][bufString];
};


#include "MyString.h"



void findFullPath(LPTSTR pszPath, LPTSTR pszFullPath);

void shellExecuteCmdLine(LPCTSTR pszCommand, LPCTSTR pszDirectory, int nShow);


struct THREAD_SHELLEXECUTE
{
	THREAD_SHELLEXECUTE(const String& sCommand, const String& sDirectory, int nShow)
		: sCommand(sCommand), sDirectory(sDirectory), nShow(nShow) {}
	
	String sCommand;
	String sDirectory;
	int    nShow;
};

DWORD WINAPI threadShellExecute(void* pParam);
