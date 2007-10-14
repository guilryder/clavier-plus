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

#include <stdarg.h>
#include "Lang.h"

extern HANDLE e_hHeap;


inline bool strIsEmpty(LPCTSTR src) { return !src || !*src; }
inline bool strIsSome(LPCTSTR  src) { return  src &&  *src; }

inline void strMove(LPTSTR dest, LPCTSTR src, int len)
{
	while (len > 0) {
		len--;
		*dest++ = *src++;
	}
}


class String
{
public:
	
	typedef TCHAR *STR;
	typedef const TCHAR *CSTR;
	
	
	operator CSTR () const { return getSafe(); }
	
	
	//----------------------------------------------------------------------
	// Constructors, destructor
	//----------------------------------------------------------------------
	
	String() : m_psz(NULL), m_buf(0) {}
	
	String(CSTR psz) { affectInit(psz); }
	
	String(const String& s) { affectInit((CSTR)s); }
	
	String(int idResource) : m_psz(NULL), m_buf(0)
	{
		loadString(idResource);
	}
	
	~String() { destroy(); }
	
	
	//----------------------------------------------------------------------
	// Affectation
	//----------------------------------------------------------------------
	
	String& operator = (const String& s)
	{
		if (&s != this) {
			affect((CSTR)s);
		}
		return *this;
	}
	
	String& operator = (CSTR psz)
	{
		if (strIsEmpty(psz)) {
			empty();
		} else if (isOverlapping(psz)) {
			const int buf = lstrlen(psz) + 1;
			if (m_buf < buf) {
				const STR pszNew = allocNew(buf);
				lstrcpy(pszNew, psz);
				destroy();
				m_psz = pszNew;
			} else {
				strMove(m_psz, psz, buf);
			}
		} else {
			affect(psz);
		}
		return *this;
	}
	
	
	//----------------------------------------------------------------------
	// Appending
	//----------------------------------------------------------------------
	
	String& operator += (const String& s)
	{
		if (this == &s) {
			if (isSome()) {
				const int len = getLength();
				const int lenAll = len + len;
				if (lenAll >= m_buf) {
					const STR pszNew = allocNew(lenAll + 1);
					lstrcpy(pszNew,       m_psz);
					lstrcpy(pszNew + len, m_psz);
				} else {
					for (int i = 0; i < len; i++) {
						m_psz[len + i] = m_psz[i];
					}
					m_psz[lenAll] = 0;
				}
			}
		} else {
			append((CSTR)s);
		}
		return *this;
	}
	
	String& operator += (CSTR psz)
	{
		append(psz);
		return *this;
	}
	
	String& operator += (TCHAR c)
	{
		appendChar(c);
		return *this;
	}
	
	
	//----------------------------------------------------------------------
	// Get
	//----------------------------------------------------------------------
	
	STR get() { return m_psz; }
	
	CSTR getSafe() const
	{
		static const TCHAR pszEmpty[] = { 0 };
		return (m_psz) ? m_psz : pszEmpty;
	}
	
	TCHAR  operator [] (int    offset) const { return getSafe()[offset]; }
	TCHAR  operator [] (size_t offset) const { return getSafe()[offset]; }
	TCHAR& operator [] (int    offset)       { return m_psz[offset]; }
	TCHAR& operator [] (size_t offset)       { return m_psz[offset]; }
	
	int getLength() const { return lstrlen(m_psz); }
	
	bool isEmpty() const { return strIsEmpty(m_psz); }
	bool isSome() const { return strIsSome(m_psz); }
	
	
	//----------------------------------------------------------------------
	// Operations
	//----------------------------------------------------------------------
	
	STR getBuffer(int buf)
	{
		reallocIfNeeded(buf);
		return m_psz;
	}
	
	void empty()
	{
		destroy();
		m_psz = NULL;
		m_buf = 0;
	}
	
	
	bool loadString(UINT id)
	{
		const STRING_RESOURCE *const pResource = ::loadStringResource(id);
		if (!pResource) {
			*m_psz = 0;
			return false;
		}
		
		pResource->copy(getBuffer(pResource->len), pResource->len);
		return true;
	}
	
	
protected:
	
	STR m_psz;
	int m_buf;
	
	
	inline void alloc(int buf) { m_psz = allocNew(buf); }
	
	inline STR allocNew(int buf)
	{
		m_buf = buf | 15;
		return bufferAlloc(m_buf);
	}
	
	void reallocIfNeeded(int buf)
	{
		if (buf > m_buf) {
			buf *= 2;
			m_psz = bufferRealloc(m_psz, buf);
			if (!m_buf) {
				*m_psz = _T('\0');
			}
			m_buf = buf;
		}
	}
	
	inline void destroy() { bufferFree(m_psz); }
	
	void affectInit(CSTR psz) { affectInit(psz, lstrlen(psz)); }
	
	void affectInit(CSTR psz, int len)
	{
		if (len <= 0) {
			m_psz = NULL;
			m_buf = 0;
		} else {
			len++;
			alloc(len);
			lstrcpyn(m_psz, psz, len);
		}
	}
	
	void affect(CSTR psz)
	{
		if (strIsEmpty(psz)) {
			empty();
		} else {
			const int buf = lstrlen(psz) + 1;
			if (m_buf < buf) {
				destroy();
				alloc(buf);
			}
			lstrcpyn(m_psz, psz, m_buf);
		}
	}
	
	void append(CSTR psz)
	{
		if (isEmpty()) {
			*this = psz;
		} else if (!strIsEmpty(psz)) {
			const int len  = getLength();
			const int buf2 = lstrlen(psz) + 2;
			reallocIfNeeded(len + buf2);
			lstrcpyn(m_psz + len, psz, buf2);
		}
	}
	
	inline void appendChar(TCHAR c)
	{
		const int len = getLength();
		reallocIfNeeded(len + 2);
		m_psz[len]     = c;
		m_psz[len + 1] = 0;
	}
	
	
	inline bool isOverlapping(CSTR psz) const
	{
		return m_psz <= psz && psz < m_psz + m_buf;
	}
	
	
protected:
	
	static STR bufferAlloc(int buf)
	{
		return (STR)HeapAlloc(e_hHeap, 0, buf * sizeof(TCHAR));
	}
	
	static void bufferFree(STR psz)
	{
		HeapFree(e_hHeap, 0, psz);
	}
	
	static STR bufferRealloc(STR psz, int buf)
	{
		return (psz)
			? (STR)HeapReAlloc(e_hHeap, 0, psz, buf * sizeof(TCHAR))
			: bufferAlloc(buf);
	}
};
