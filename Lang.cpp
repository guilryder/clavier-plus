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


#include "StdAfx.h"
#include "Lang.h"

extern HINSTANCE e_hInst;
extern HWND      e_hdlgModal;

int e_lang;


static LANGID s_langID;

static const LANGID s_aLangID[] =
{
	MAKELANGID(LANG_FRENCH,     SUBLANG_FRENCH),
	MAKELANGID(LANG_ENGLISH,    SUBLANG_ENGLISH_US),
	MAKELANGID(LANG_GERMAN,     SUBLANG_GERMAN),
	MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN),
};


static void* loadResource(UINT id, LPCTSTR pszType);



int getDefaultLanguage()
{
	const LANGID langID = PRIMARYLANGID(LANGIDFROMLCID(GetUserDefaultLCID()));
	for (int lang = 0; lang < nbArray(s_aLangID); lang++) {
		if (PRIMARYLANGID(s_aLangID[lang]) == langID) {
			return lang;
		}
	}
	return langEN;
}


void setLanguage(int lang)
{
	e_lang = lang;
	s_langID = s_aLangID[lang];
}



void* loadResource(UINT id, LPCTSTR pszType)
{
	const HRSRC hResource = FindResourceEx(e_hInst, pszType,
		MAKEINTRESOURCE(id), s_langID);
	VERIFP(hResource, NULL);
	
	const HGLOBAL hGlobal = LoadResource(e_hInst, hResource);
	VERIFP(hGlobal, NULL);
	
	return LockResource(hGlobal);
}


const STRING_RESOURCE* loadStringResource(UINT id)
{
	const STRING_RESOURCE *pResource =
		(const STRING_RESOURCE*)loadResource(id / 16 + 1, RT_STRING);
	VERIFP(pResource, NULL);
	
	for (id &= 15; id > 0; id--) {
		pResource = (STRING_RESOURCE*)(pResource->wsz + pResource->len);
	}
	
	return pResource;
}


void loadString(UINT id, LPTSTR psz, int buf)
{
	const STRING_RESOURCE *const pResource = loadStringResource(id);
	if (buf > pResource->len) {
		buf = pResource->len + 1;
	}
	
	if (pResource) {
		pResource->copy(psz, buf);
	} else {
		*psz = _T('\0');
	}
}


int dialogBox(UINT id, HWND hwndParent, DLGPROC prc, LPARAM lInitParam)
{
	const HWND hdlgModalOld = e_hdlgModal;
	
	const int iResult = DialogBoxIndirectParam(e_hInst,
		(const DLGTEMPLATE*)loadResource(id, RT_DIALOG),
		hwndParent, prc, lInitParam);
	
	e_hdlgModal = hdlgModalOld;
	return iResult;
}


HMENU loadMenu(UINT id)
{
	return LoadMenuIndirect((const MENUTEMPLATE*)loadResource(id, RT_MENU));
}


HBITMAP loadBitmap(UINT id)
{
	BITMAPINFO *const pbi = (BITMAPINFO*)loadResource(id, RT_BITMAP);
	VERIFP(pbi, NULL);
	
	const HDC hdc = GetDC(NULL);
	const HBITMAP hBitmap = CreateDIBitmap(hdc, &pbi->bmiHeader, CBM_INIT,
		pbi->bmiColors + pbi->bmiHeader.biClrUsed,
		pbi, DIB_RGB_COLORS);
	ReleaseDC(NULL, hdc);
	
	return hBitmap;
}
