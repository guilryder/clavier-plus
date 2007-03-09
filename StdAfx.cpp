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


#ifndef _DEBUG

extern HANDLE e_hHeap;

void* operator new(size_t size)
{
	return HeapAlloc(e_hHeap, 0, size);
}

void operator delete(void* p)
{
	HeapFree(e_hHeap, 0, p);
}

#endif
