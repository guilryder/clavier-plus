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

// Implement the intrinsices used by the application.
// Needed to remove all dependencies to Visual C++ runtime DLLs.


extern "C" void* memset(void* dest, int value, size_t size);
extern "C" void* memcpy(void* dest, const void* src, size_t size);

#ifndef _DEBUG

#pragma function(memset)
#pragma function(memcpy)

typedef unsigned char BYTE;

void* memset(void* dst, int value, size_t size) {
	BYTE *pdst = (BYTE*)dst;
	while (size > 0) {
		size--;
		*pdst++ = (BYTE)value;
	}
	
	return dst;
}

void* memcpy(void* dst, const void* src, size_t size) {
	BYTE *pdst = (BYTE*)dst;
	const BYTE *psrc = (const BYTE*)src;
	while (size > 0) {
		size--;
		*pdst++ = *psrc++;
	}
	
	return dst;
}

extern "C" int atexit(void (__cdecl*)()) {
	return 0;
}

#endif  // !_DEBUG
