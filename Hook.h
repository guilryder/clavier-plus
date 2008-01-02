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

namespace keyboard_hook {

// Installs the keyboard hook. Should be called only once, at process initialization.
void install();

// Uninstalls the keyboard hook. Should be called only once, before the process terminates.
void uninstall();

#define WM_CLAVIER_KEYDOWN  (WM_USER + 102)
#define WM_CLAVIER_KEYUP  (WM_USER + 103)

// Sets the window to which the keyboard hook should redirect (post) all keyboard messages.
// Used by keystroke entering controls to intercept all keys, including the Windows-reserved ones.
// The window will be posted WM_CLAVIER_KEYDOWN and WM_CLAVIER_KEYUP messages, with arguments:
//   wParam: the virtual key code
//   lParam: the special keys mask
//     special_key_index * 2: "left key is down" bit
//     special_key_index * 2 + 1: "right key is down" bit
// We don't use WM_KEYDOWN and WM_KEYUP because the arguments don't have the same semantics.
//
// Args:
//   hwnd: The handle of the window to which keyboard messages should be redirected. If null,
//     disables redirection.
void setCatchAllKeysWindow(HWND hwnd);

}  // keyboard_hook namespace
