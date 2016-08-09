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
#include "I18n.h"

extern HINSTANCE e_hInst;
extern HWND e_hdlgModal;

namespace i18n {

static int s_lang;


static LANGID s_lang_id;

static const LANGID s_lang_ids[] = {
	MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN),
	MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
	MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH),
	MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN),
	MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT),
};


static void* loadResource(UINT id, LPCTSTR type);


int getLanguage() {
	return s_lang;
}

void setLanguage(int lang) {
	s_lang = lang;
	s_lang_id = s_lang_ids[lang];
}


int getDefaultLanguage() {
	const LANGID lang_id = PRIMARYLANGID(LANGIDFROMLCID(GetUserDefaultLCID()));
	for (int lang = 0; lang < arrayLength(s_lang_ids); lang++) {
		if (PRIMARYLANGID(s_lang_ids[lang]) == lang_id) {
			return lang;
		}
	}
	return langEN;
}


void* loadResource(UINT id, LPCTSTR type) {
	const HRSRC hResource = FindResourceEx(e_hInst, type, MAKEINTRESOURCE(id), s_lang_id);
	VERIFP(hResource, NULL);
	
	const HGLOBAL hGlobal = LoadResource(e_hInst, hResource);
	VERIFP(hGlobal, NULL);
	
	return LockResource(hGlobal);
}


const STRING_RESOURCE* loadStringResource(UINT id) {
	const STRING_RESOURCE* resource =
		reinterpret_cast<const STRING_RESOURCE*>(loadResource(id / 16 + 1, RT_STRING));
	VERIFP(resource, NULL);
	
	for (id &= 15; id > 0; id--) {
		resource = reinterpret_cast<const STRING_RESOURCE*>(resource->strbuf + resource->length);
	}
	
	return resource;
}


void loadString(UINT id, LPTSTR strbuf, int buffer_length) {
	const STRING_RESOURCE* const resource = loadStringResource(id);
	if (buffer_length > resource->length) {
		buffer_length = resource->length + 1;
	}
	
	if (resource) {
		resource->copy(strbuf, buffer_length);
	} else {
		*strbuf = _T('\0');
	}
}


INT_PTR dialogBox(UINT id, HWND hwnd_parent, DLGPROC window_proc, LPARAM init_param) {
	const HWND hdlgModalOld = e_hdlgModal;
	
	const INT_PTR iResult = DialogBoxIndirectParam(
		e_hInst,
		reinterpret_cast<const DLGTEMPLATE*>(loadResource(id, RT_DIALOG)),
		hwnd_parent, window_proc, init_param);
	
	e_hdlgModal = hdlgModalOld;
	return iResult;
}


HMENU loadMenu(UINT id) {
	return LoadMenuIndirect(reinterpret_cast<const MENUTEMPLATE*>(loadResource(id, RT_MENU)));
}


HBITMAP loadBitmap(UINT id) {
	BITMAPINFO *const pbi = reinterpret_cast<BITMAPINFO*>(loadResource(id, RT_BITMAP));
	VERIFP(pbi, NULL);
	
	const HDC hdc = GetDC(NULL);
	const HBITMAP hBitmap = CreateDIBitmap(
		hdc, &pbi->bmiHeader, CBM_INIT,
		pbi->bmiColors + pbi->bmiHeader.biClrUsed,
		pbi, DIB_RGB_COLORS);
	ReleaseDC(NULL, hdc);
	
	return hBitmap;
}

}  // i18n namespace
