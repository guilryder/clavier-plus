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

#include "Global.h"
#include "Keyboard.h"

#define WM_KEYSTROKE     (WM_USER + 100)
#define WM_GETFILEICON   (WM_USER + 101)


enum
{
	tokLanguageName,
	
	tokShortcut,
	tokCode,
	tokDistinguishLeftRight,
	tokDescription,
	tokCommand,
	tokText,
	tokDirectory,
	tokWindow,
	tokSupportFileOpen,
	tokPrograms,
	tokAllProgramsBut,
	tokNone,
	tokLanguage,
	tokSize,
	tokColumns,
	tokSorting,
	
	tokShowNormal,
	tokShowMinimize,
	tokShowMaximize,
	
	tokWin,
	tokCtrl,
	tokShift,
	tokAlt,
	tokLeft,
	tokRight,
	
	tokConditionCapsLock,
	tokConditionNumLock,
	tokConditionScrollLock,
	tokConditionYes,
	tokConditionNo,
	
	tokNotFound
};


// Shortcut list, NULL if the dialog box is not displayed
extern HWND e_hlst;
