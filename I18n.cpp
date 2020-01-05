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
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "StdAfx.h"
#include "I18n.h"
#include "MyString.h"

extern HINSTANCE e_hInst;
extern HWND e_hdlgModal;

namespace i18n {

static int s_lang;


static LANGID s_lang_id;

static const LANGID s_lang_ids[] = {
	MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
	MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN),
	MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
	MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH),
	MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN),
	MAKELANGID(LANG_DUTCH, SUBLANG_DUTCH_BELGIAN),
	MAKELANGID(LANG_POLISH, SUBLANG_DEFAULT),
	MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN),
	MAKELANGID(LANG_SLOVAK, SUBLANG_DEFAULT),
	MAKELANGID(LANG_GREEK, SUBLANG_DEFAULT),
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


void formatInteger(int number, String* output) {
	const LCID locale = LOCALE_USER_DEFAULT;
	
	static NUMBERFMT format;
	
	// Populate format once for all.
	static bool format_initialized = false;
	if (!format_initialized) {
		format_initialized = true;
		
		format.NumDigits = 0;
		format.LeadingZero = TRUE;
		format.lpDecimalSep = _T(".");  // unused
		
		// Number groups. Convert the LOCALE_SGROUPING string ("3;2;0") into a number (320).
		TCHAR grouping_buf[10];
		GetLocaleInfo(locale, LOCALE_SGROUPING, grouping_buf, arrayLength(grouping_buf));
		format.Grouping = parseNumberGroupingString(grouping_buf);
		
		static TCHAR thousand_sep_buf[4];
		GetLocaleInfo(locale, LOCALE_STHOUSAND, thousand_sep_buf, arrayLength(thousand_sep_buf));
		format.lpThousandSep = thousand_sep_buf;
		
		GetLocaleInfo(locale, LOCALE_RETURN_NUMBER | LOCALE_INEGNUMBER,
			reinterpret_cast<LPTSTR>(&format.NegativeOrder),
			sizeof(format.NegativeOrder) / sizeof(TCHAR));
	}
	
	TCHAR number_buf[bufIntegerCount];
	wsprintf(number_buf, _T("%d"), number);
	GetNumberFormat(locale, 0, number_buf, &format, output->getBuffer(bufIntegerCount), bufIntegerCount);
}


int parseNumberGroupingString(LPCTSTR input) {
	int result = 0;
	LPCTSTR current_char;
	for (current_char = input; *current_char; current_char++) {
		if (_T('0') <= *current_char && *current_char <= _T('9')) {
			result = (result * 10) + (*current_char - _T('0'));
		}
	}
	if (*input && (current_char[-1] == _T('0'))) {
		result /= 10;
	} else {
		result *= 10;
	}
	return result;
}

}  // i18n namespace
