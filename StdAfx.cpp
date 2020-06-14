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

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "msi.lib")
#pragma comment(lib, "propsys.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shlwapi.lib")

#ifndef _DEBUG

// Implement the new and delete operators with a Windows heap.
// Needed to remove all dependencies to Visual C++ runtime DLLs.

extern HANDLE e_heap;

void* operator new(size_t size) {
	return HeapAlloc(e_heap, 0, size);
}

void operator delete(void* p) {
	HeapFree(e_heap, 0, p);
}

void operator delete(void* p, size_t) {
	::operator delete(p);
}

void* operator new[](size_t size) {
	return ::operator new(size);
}

void operator delete[](void* p) {
	return ::operator delete(p);
}

void operator delete[](void* p, size_t) {
	return ::operator delete(p);
}

#endif  // !_DEBUG
