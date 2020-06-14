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


// String: wrapper for a TCHAR character buffer.


#pragma once

#include <stdarg.h>
#include "I18n.h"

namespace MyStringTest {
class StringTest;
}

extern HANDLE e_heap;


// Indicates whether a character buffer is null or empty.
inline bool strIsEmpty(LPCTSTR src) {
	return !src || !*src;
}

// Copies a given number of character from a string to a character buffer.
// The destination character buffer can overlap with the source one.
inline void strMove(LPTSTR dest, LPCTSTR src, int length) {
	while (length > 0) {
		length--;
		*dest++ = *src++;
	}
}


class String {
	friend class ::MyStringTest::StringTest;
	
public:
	
	typedef TCHAR* STR;
	typedef const TCHAR* CSTR;
	
	operator CSTR () const {
		return getSafe();
	}
	
	int getBufferSize() const {
		return m_buf_length;
	}
	
	//----------------------------------------------------------------------
	// Constructors, destructor
	//----------------------------------------------------------------------
	
	String() : m_strbuf(nullptr), m_buf_length(0) {
	}
	
	String(CSTR strbuf) {
		affectInit(strbuf);
	}
	
	String(const String& str) {
		affectInit((CSTR)str);
	}
	
	String(int resource_id) : m_strbuf(nullptr), m_buf_length(0) {
		loadString(resource_id);
	}
	
	~String() {
		destroy();
	}
	
	//----------------------------------------------------------------------
	// Affectation
	//----------------------------------------------------------------------
	
	String& operator = (const String& str) {
		if (&str != this) {
			affect((CSTR)str);
		}
		return *this;
	}
	
	String& operator = (CSTR strbuf) {
		if (strIsEmpty(strbuf)) {
			empty();
		} else if (isOverlapping(strbuf)) {
			const int buf_length = lstrlen(strbuf) + 1;
			if (m_buf_length < buf_length) {
				const STR strbuf_new = allocNew(buf_length);
				lstrcpy(strbuf_new, strbuf);
				destroy();
				m_strbuf = strbuf_new;
			} else {
				strMove(m_strbuf, strbuf, buf_length);
			}
		} else {
			affect(strbuf);
		}
		return *this;
	}
	
	//----------------------------------------------------------------------
	// Appending
	//----------------------------------------------------------------------
	
	String& operator += (const String& str) {
		if (this == &str) {
			if (isSome()) {
				const int old_length = getLength();
				const int new_length = old_length + old_length;
				if (new_length >= m_buf_length) {
					const STR strbuf_new = allocNew(new_length + 1);
					lstrcpy(strbuf_new, m_strbuf);
					lstrcpy(strbuf_new + old_length, m_strbuf);
				} else {
					for (int i = 0; i < old_length; i++) {
						m_strbuf[old_length + i] = m_strbuf[i];
					}
					m_strbuf[new_length] = 0;
				}
			}
		} else {
			append((CSTR)str);
		}
		return *this;
	}
	
	String& operator += (CSTR strbuf) {
		append(strbuf);
		return *this;
	}
	
	String& operator += (TCHAR c) {
		appendChar(c);
		return *this;
	}
	
	//----------------------------------------------------------------------
	// Get
	//----------------------------------------------------------------------
	
	STR get() {
		return m_strbuf;
	}
	
	CSTR getSafe() const {
		static constexpr TCHAR empty_string[] = { 0 };
		return (m_strbuf) ? m_strbuf : empty_string;
	}
	
	TCHAR& operator [] (int offset) {
		return m_strbuf[offset];
	}
	
	TCHAR& operator [] (size_t offset) {
		return m_strbuf[offset];
	}
	
	int getLength() const {
		return lstrlen(m_strbuf);
	}
	
	bool isEmpty() const {
		return strIsEmpty(m_strbuf);
	}
	
	bool isSome() const {
		return m_strbuf && *m_strbuf;
	}
	
	//----------------------------------------------------------------------
	// Operations
	//----------------------------------------------------------------------
	
	STR getBuffer(int buf_length) {
		reallocIfNeeded(buf_length);
		return m_strbuf;
	}
	
	void empty() {
		destroy();
		m_strbuf = nullptr;
		m_buf_length = 0;
	}
	
	
	bool loadString(UINT id) {
		const i18n::STRING_RESOURCE *const resource = i18n::loadStringResource(id);
		if (!resource) {
			empty();
			return false;
		}
		
		int buf_length = resource->length + 1;
		resource->copy(getBuffer(buf_length), buf_length);
		return true;
	}
	
private:
	
	STR m_strbuf;
	int m_buf_length;
	
	inline void alloc(int buf_length) {
		m_strbuf = allocNew(buf_length);
	}
	
	inline STR allocNew(int buf_length) {
		m_buf_length = buf_length | 15;  // Allocate up to 15 bytes more than requested.
		return bufferAlloc(m_buf_length);
	}
	
	void reallocIfNeeded(int buf_length) {
		if (buf_length > m_buf_length) {
			buf_length *= 2;
			m_strbuf = bufferRealloc(m_strbuf, buf_length);
			if (!m_buf_length) {
				*m_strbuf = 0;
			}
			m_buf_length = buf_length;
		}
	}
	
	inline void destroy() {
		bufferFree(m_strbuf);
	}
	
	void affectInit(CSTR strbuf) {
		affectInit(strbuf, lstrlen(strbuf));
	}
	
	void affectInit(CSTR strbuf, int length) {
		if (length <= 0) {
			m_strbuf = nullptr;
			m_buf_length = 0;
		} else {
			length++;
			alloc(length);
			lstrcpyn(m_strbuf, strbuf, length);
		}
	}
	
	void affect(CSTR strbuf) {
		if (strIsEmpty(strbuf)) {
			empty();
		} else {
			const int buf_length = lstrlen(strbuf) + 1;
			if (m_buf_length < buf_length) {
				destroy();
				alloc(buf_length);
			}
			lstrcpyn(m_strbuf, strbuf, m_buf_length);
		}
	}
	
	void append(CSTR strbuf) {
		if (isEmpty()) {
			*this = strbuf;
		} else if (!strIsEmpty(strbuf)) {
			const int length = getLength();
			const int appended_buf_length = lstrlen(strbuf) + 1;
			reallocIfNeeded(length + appended_buf_length);
			lstrcpyn(m_strbuf + length, strbuf, appended_buf_length);
		}
	}
	
	inline void appendChar(TCHAR chr) {
		const int length = getLength();
		reallocIfNeeded(length + 2);
		m_strbuf[length] = chr;
		m_strbuf[length + 1] = 0;
	}
	
	inline bool isOverlapping(CSTR strbuf) const {
		return m_strbuf <= strbuf && strbuf < m_strbuf + m_buf_length;
	}
	
private:
	
	static STR bufferAlloc(int buf_length) {
		return static_cast<STR>(
			HeapAlloc(e_heap, /* dwFlags= */ 0, buf_length * sizeof(TCHAR)));
	}
	
	static void bufferFree(STR strbuf) {
		HeapFree(e_heap, /* dwFlags= */ 0, strbuf);
	}
	
	static STR bufferRealloc(STR strbuf, int buf_length) {
		return strbuf
			? static_cast<STR>(
				HeapReAlloc(e_heap, /* dwFlags= */ 0, strbuf, buf_length * sizeof(TCHAR)))
			: bufferAlloc(buf_length);
	}
};
