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


#pragma once

#include "Global.h"

#ifndef OPENFILENAME_SIZE_VERSION_400
#define OPENFILENAME_SIZE_VERSION_400  sizeof(OPENFILENAME)
#endif  // !OPENFILENAME_SIZE_VERSION_400

#define WM_KEYSTROKE  (WM_USER + 100)
#define WM_GETFILEICON  (WM_USER + 101)


// Each token is associated to a part of the IDS_TOKENS resource string. IDS_TOKENS is a ';'
// separated string whose parts are in the same other as the values of this enum.
enum {
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
	
	tokUsageCount,
	
	tokNotFound
};


namespace app {
	void initialize();
	void terminate();
}  // app namespace
