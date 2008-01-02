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
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#pragma once

namespace dialogs {

extern HWND e_hdlgMain;

// Performs initialization for the current language. Should be called for each available language.
void initializeCurrentLanguage();

// Displays the main window as a modal dialog box.
//
// Args:
//   initial_command: If not zero, the ID of the WM_COMMAND command to post initially. Used, for
//     example, to open the "add shortcut" dialog box initially.
//
// Returns:
//   The ID of the last clicked button: ID_OK, ID_CANCEL, IDCCMD_QUIT, IDCCMD_LANGUAGE.
UINT showMainDialogModal(UINT initial_command);

}  // dialogs namespace
