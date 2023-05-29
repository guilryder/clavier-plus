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
	
	String() : m_strbuf(nullptr), m_buf_length(0) {}
	
	String(CSTR strbuf) : String(strbuf, lstrlen(strbuf)) {}
	
	String(CSTR strbuf, int length) {
		if (length <= 0) {
			m_strbuf = nullptr;
			m_buf_length = 0;
		} else {
			length++;
			alloc(length);
			StringCchCopy(m_strbuf, length, strbuf);
		}
	}
	
	String(const String& str) : String(CSTR(str)) {}
	
	String(String&& str) {
		m_strbuf = str.m_strbuf;
		m_buf_length = str.m_buf_length;
		str.m_strbuf = nullptr;
		str.m_buf_length = 0;
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
			affect(CSTR(str));
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
				StringCchCopy(strbuf_new, buf_length, strbuf);
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
	
	String& operator += (CSTR input_strbuf) {
		if (isEmpty()) {
			*this = input_strbuf;
		} else if (!strIsEmpty(input_strbuf)) {
			if (isOverlapping(input_strbuf)) {
				const int old_length = getLength();
				const int input_self_index = int(input_strbuf - m_strbuf);
				const int input_length = old_length - input_self_index;
				reallocIfNeeded(old_length + input_length + 1);
				for (int i = 0; i < input_length; i++) {
					m_strbuf[old_length + i] = m_strbuf[input_self_index + i];
				}
				m_strbuf[old_length + input_length] = 0;
			} else {
				const int length = getLength();
				const int input_buf_length = lstrlen(input_strbuf) + 1;
				reallocIfNeeded(length + input_buf_length);
				StringCchCopy(m_strbuf + length, input_buf_length, input_strbuf);
			}
		}
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
		static constexpr TCHAR kEmptyString[] = { 0 };
		return (m_strbuf) ? m_strbuf : kEmptyString;
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
	
	void affect(CSTR strbuf) {
		if (strIsEmpty(strbuf)) {
			empty();
		} else {
			const int buf_length = lstrlen(strbuf) + 1;
			if (m_buf_length < buf_length) {
				destroy();
				alloc(buf_length);
			}
			StringCchCopy(m_strbuf, m_buf_length, strbuf);
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
		return STR(HeapAlloc(e_heap, /* dwFlags= */ 0, buf_length * sizeof(TCHAR)));
	}
	
	static void bufferFree(STR strbuf) {
		HeapFree(e_heap, /* dwFlags= */ 0, strbuf);
	}
	
	static STR bufferRealloc(STR strbuf, int buf_length) {
		return strbuf
			? STR(HeapReAlloc(e_heap, /* dwFlags= */ 0, strbuf, buf_length * sizeof(TCHAR)))
			: bufferAlloc(buf_length);
	}
};

#ifdef _DEBUG

inline String StringPrintf(LPCTSTR format, ...) {
	TCHAR result[1024];
	va_list args;
	va_start(args, format);
	vswprintf(result, arrayLength(result), format, args);
	va_end(args);
	return result;
}

#endif // _DEBUG
