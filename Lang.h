// Clavier+
// Keyboard shortcuts manager
//
// Copyright (C) 2000-2008 Guillaume Ryder
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


enum {
	langFR,
	langEN,
	langDE,
	langPT,
	langCount
};


extern int e_lang;

int getDefaultLanguage();
void setLanguage(int lang);


#pragma warning(disable: 4200)
struct STRING_RESOURCE {
	WORD len;
	WCHAR wsz[];
	
	void copy(LPTSTR pszDest, int buf) const {
#ifdef UNICODE
		lstrcpyn(pszDest, wsz, buf);
#else
		WideCharToMultiByte(CP_ACP, 0, wsz, len, pszDest, buf, NULL, NULL);
		pszDest[(len < buf) ? len : buf - 1] = _T('\0');
#endif
	}
};
#pragma warning(default: 4200)


const STRING_RESOURCE* loadStringResource(UINT id);

void loadString(UINT id, LPTSTR psz, int buf);

#define loadStringAuto(id, pszBuffer) \
	loadString(id, pszBuffer, nbArray(pszBuffer))

#define loadStringAutoRet(id, pszBuffer) \
	(loadStringAuto(id, pszBuffer), pszBuffer)


int dialogBox(UINT id, HWND hwndParent, DLGPROC prc, LPARAM lInitParam = 0);

HMENU loadMenu(UINT id);

HBITMAP loadBitmap(UINT id);
