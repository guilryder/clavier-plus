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


inline LPTSTR strFind(LPCTSTR psz, LPCTSTR pszSearch) { return StrStr(psz, pszSearch); }
inline LPTSTR strFind(LPCTSTR psz, TCHAR c)
{
	for (;;) {
		if (!*psz)
			return NULL;
		if (*psz == c)
			return (LPTSTR)psz;
		psz++;
	}
}


class String
{
public:
	
	typedef       TCHAR       *STR;
	typedef const TCHAR       *CSTR;
	
	
	operator CSTR () const { return getSafe(); }
	
	
	//----------------------------------------------------------------------
	// Constructors, destructor
	//----------------------------------------------------------------------
	
	String() : m_psz(NULL), m_buf(0) {}
	
	String(CSTR psz) { affectInit(psz); }
	
	String(CSTR psz, int len) { affectInit(psz, len); }
	
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
		if (&s != this)
			affect((CSTR)s);
		return *this;
	}
	
	String& operator = (CSTR psz)
	{
		if (strIsEmpty(psz))
			empty();
		else if (isOverlapping(psz)) {
			const int buf = lstrlen(psz) + 1;
			if (m_buf < buf) {
				const STR pszNew = allocNew(buf);
				lstrcpy(pszNew, psz);
				destroy();
				m_psz = pszNew;
			}else
				strMove(m_psz, psz, buf);
		}else
			affect(psz);
		return *this;
	}
	
	
	String& operator = (TCHAR c)
	{
		reallocIfNeeded(2);
		m_psz[0] = c;
		m_psz[1] = 0;
		return *this;
	}
	
	
	void set(CSTR psz, int len)
	{
		if (strIsEmpty(psz))
			empty();
		else{
			len++;
			if (isOverlapping(psz)) {
				if (m_buf < len) {
					const STR pszNew = allocNew(len);
					lstrcpyn(pszNew, psz, len);
					destroy();
					m_psz = pszNew;
				}else{
					const int bufTrue = lstrlen(psz) + 1;
					if (len > bufTrue)
						len = bufTrue;
					strMove(m_psz, psz, bufTrue);
					m_psz[len] = 0;
				}
			}else{
				reallocIfNeeded(len);
				lstrcpyn(m_psz, psz, len);
			}
		}
	}
	
	void attach(STR psz)
	{
		empty();
		m_psz = psz;
		m_buf = lstrlen(psz) + 1;
	}
	
	void attach(STR psz, int buf)
	{
		empty();
		m_psz = psz;
		m_buf = buf;
	}
	
	void attach(String& rsFrom)
	{
		empty();
		rsFrom.m_psz = rsFrom.m_psz;
		rsFrom.m_buf = rsFrom.m_buf;
		rsFrom.m_psz = NULL;
		rsFrom.m_buf = 0;
	}
	
	STR detach()
	{
		const STR psz = m_psz;
		m_psz = NULL;
		m_buf = 0;
		return psz;
	}
	
	
	//----------------------------------------------------------------------
	// Appending
	//----------------------------------------------------------------------
	
	String& operator += (const String& s)
	{
		if (this == &s) {
			if (isSome()) {
				const int len    = getLength();
				const int lenAll = len + len;
				if (lenAll >= m_buf) {
					const STR pszNew = allocNew(lenAll + 1);
					lstrcpy(pszNew,       m_psz);
					lstrcpy(pszNew + len, m_psz);
				}else{
					for (int i = 0; i < len; i++)
						m_psz[len + i] = m_psz[i];
					m_psz[lenAll] = 0;
				}
			}
		}else
			append((CSTR)s);
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
	int getBufferSize() const { return m_buf; }
	
	CSTR getSafe() const
	{
		static const TCHAR pszEmpty[] = { 0 };
		return (m_psz) ? m_psz : pszEmpty;
	}
	
	CSTR getSafer() const
	{
		static const TCHAR pszEmpty[] = { 0 };
		return (this && m_psz) ? m_psz : pszEmpty;
	}
	
	TCHAR  operator [] (int    offset) const { return getSafe()[offset]; }
	TCHAR  operator [] (size_t offset) const { return getSafe()[offset]; }
	TCHAR& operator [] (int    offset)       { return m_psz[offset]; }
	TCHAR& operator [] (size_t offset)       { return m_psz[offset]; }
	
	int getLength() const { return lstrlen(m_psz); }
	
	bool isEmpty() const { return strIsEmpty(m_psz); }
	bool isSome() const { return strIsSome(m_psz); }
	
	
	CSTR mid(int begin) const
	{
		if (begin < 0)  return m_psz;
		const int lenString = getLength();
		if (begin >= lenString)  begin = lenString;
		return m_psz + begin;
	}
	
	String mid(int begin, int len) const
	{
		const int lenString = getLength();
		if (begin < 0) {
			len += begin;
			begin = 0;
		}else if (begin >= lenString)
			begin = lenString;
		return String(m_psz + begin, len);
	}
	
	String left(int len) const { return String(m_psz, len); }
	
	CSTR right(int len) const
	{
		if (len < 0)
			len = 0;
		const int lenString = getLength();
		return m_psz + lenString - min(lenString, len);
	}
	
	
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
	
	
	void makeLower() { CharLower(m_psz); }
	void makeUpper() { CharUpper(m_psz); }
	
	
	//----------------------------------------------------------------------
	// Comparison
	//----------------------------------------------------------------------
	
	bool operator == (CSTR psz) const { return !lstrcmp(m_psz, psz); }
	bool operator != (CSTR psz) const { return  lstrcmp(m_psz, psz) != 0; }
	bool operator <= (CSTR psz) const { return  lstrcmp(m_psz, psz) <= 0; }
	bool operator <  (CSTR psz) const { return  lstrcmp(m_psz, psz) <  0; }
	bool operator >= (CSTR psz) const { return  lstrcmp(m_psz, psz) >= 0; }
	bool operator >  (CSTR psz) const { return  lstrcmp(m_psz, psz) >  0; }
	
	int compare      (CSTR psz) const { return  lstrcmp (m_psz, psz); }
	int compareNoCase(CSTR psz) const { return  lstrcmpi(m_psz, psz); }
	
	
	//----------------------------------------------------------------------
	// Search & remplace
	//----------------------------------------------------------------------
	
	STR find(TCHAR c) const { return strFind(m_psz, c); }
	STR find(CSTR psz) const { return strFind(m_psz, psz); }
	
	int findIndex(TCHAR c) const
	{
		const STR pszFound = find(c);
		return (pszFound) ? (int)(pszFound - m_psz) : -1;
	}
	
	int findIndex(CSTR psz) const
	{
		const STR pszFound = find(psz);
		return (pszFound) ? (int)(pszFound - m_psz) : -1;
	}
	
	void replace(TCHAR cOld, TCHAR cNew)
	{
		VERIFV(isSome() && cOld != cNew);
		for (size_t i = 0; m_psz[i]; i++)
			if (m_psz[i] == cOld)
				m_psz[i] = cNew;
	}
	
	void replaceTo(String& rsDest, TCHAR cOld, TCHAR cNew) const
	{
		if (isSome() && cOld != cNew) {
			rsDest.alloc(getLength() + 1);
			for (size_t i = 0; m_psz[i]; i++)
				rsDest.m_psz[i] = (m_psz[i] == cOld) ? cNew : m_psz[i];
		}else
			rsDest.empty();
	}
	
	
	void replace(CSTR pszOld, CSTR pszNew)
	{
		VERIFV(isSome() && strIsSome(pszOld));
		const int lenOld = lstrlen(pszOld);
		const int lenNew = lstrlen(pszNew);
		if (lenOld >= lenNew) {
			CSTR pszFrom = m_psz;
			STR  pszTo   = m_psz;
			for (;;) {
				const CSTR pszFromNew = strFind(pszFrom, pszOld);
				if (pszFromNew) {
					const int diff = (int)(pszFromNew - pszFrom);
					strMove(pszTo, pszFrom, diff);
					pszTo += diff;
					lstrcpy(pszTo, pszNew);
					pszTo += lenNew;
					pszFrom = pszFromNew + lenOld;
				}else{
					strMove(pszTo, pszFrom, lstrlen(pszFrom) + 1);
					break;
				}
			}
			
		}else{
			
			// Compute new buffer size
			int bufNew = 1;
			bool bDirty = false;
			const int lenDiff = lenNew - lenOld;
			CSTR pszFrom = m_psz;
			for (;;) {
				const CSTR pszFromNew = strFind(pszFrom, pszOld);
				if (!pszFromNew) {
					bufNew += lstrlen(pszFrom);
					break;
				}
				bufNew  += (int)(pszFromNew - pszFrom) + lenDiff;
				pszFrom  = pszFromNew + lenOld;
				bDirty = true;
			}
			VERIFV(bDirty);
			
			// Allocate new buffer
			const STR pszBufferNew = allocNew(bufNew);
			
			// Do replace
			pszFrom = m_psz;
			STR pszTo = pszBufferNew;
			for (;;) {
				const CSTR pszFromNew = strFind(pszFrom, pszOld);
				if (pszFromNew) {
					const int diff = (int)(pszFromNew - pszFrom);
					strMove(pszTo, pszFrom, diff);
					pszTo += diff;
					lstrcpy(pszTo, pszNew);
					pszTo += lenNew;
					pszFrom = pszFromNew + lenOld;
				}else{
					strMove(pszTo, pszFrom, lstrlen(pszFrom) + 1);
					break;
				}
			}
			
			replaceBy(pszBufferNew);
		}
	}
	
	
	void replaceTo(String& rsDest, CSTR pszOld, CSTR pszNew) const
	{
		if (isSome() && strIsSome(pszOld)) {
			const int lenOld = lstrlen(pszOld);
			const int lenNew = lstrlen(pszNew);
			
			// Compute new buffer size
			int bufNew = 1;
			bool bDirty = false;
			const int lenDiff = lenNew - lenOld;
			CSTR pszFrom = m_psz;
			for (;;) {
				const CSTR pszFromNew = strFind(pszFrom, pszOld);
				if (!pszFromNew) {
					bufNew += lstrlen(pszFrom);
					break;
				}
				bufNew  += (int)(pszFromNew - pszFrom) + lenDiff;
				pszFrom  = pszFromNew + lenOld;
				bDirty = true;
			}
			VERIFV(bDirty);
			
			// Do replace
			pszFrom = m_psz;
			STR pszTo = rsDest.getBuffer(bufNew);
			for (;;) {
				const CSTR pszFromNew = strFind(pszFrom, pszOld);
				if (pszFromNew) {
					const int diff = (int)(pszFromNew - pszFrom);
					strMove(pszTo, pszFrom, diff);
					pszTo += diff;
					lstrcpy(pszTo, pszNew);
					pszTo += lenNew;
					pszFrom = pszFromNew + lenOld;
				}else{
					strMove(pszTo, pszFrom, lstrlen(pszFrom) + 1);
					break;
				}
			}
			
		}else
			rsDest = *this;
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
			if (!m_buf)
				*m_psz = _T('\0');
			m_buf = buf;
		}
	}
	
	inline void destroy() { bufferFree(m_psz); }
	
	inline void replaceBy(STR pszNew)
	{
		destroy();
		m_psz = pszNew;
	}
	
	
	void affectInit(CSTR psz) { affectInit(psz, lstrlen(psz)); }
	
	void affectInit(CSTR psz, int len)
	{
		if (len <= 0) {
			m_psz = NULL;
			m_buf = 0;
		}else{
			len++;
			alloc(len);
			lstrcpyn(m_psz, psz, len);
		}
	}
	
	void affect(CSTR psz)
	{
		if (strIsEmpty(psz))
			empty();
		else{
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
		if (isEmpty())
			*this = psz;
		else if (!strIsEmpty(psz)) {
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
