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
#include "../App.h"

#include <signal.h>

namespace testing {

TEST_MODULE_INITIALIZE(clavierSetUp) {
	// Translate Win32 exceptions into std::runtime_error.
	_set_se_translator([](UINT, PEXCEPTION_POINTERS) {
		throw std::runtime_error("Win32 exception");
	});
	
	// Set e_hInst to the tests DLL instead of the test runner process.
	e_hInst = NULL;
	GetModuleHandleEx(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
		reinterpret_cast<LPCTSTR>(clavierSetUp),
		&e_hInst);

	app::initialize();
}

TEST_MODULE_CLEANUP(clavierTearDown) {
	app::terminate();
}


int normalizeCompareResult(int result) {
	if (result < 0) {
		return -1;
	} else if (result > 0) {
		return 1;
	} else {
		return 0;
	}
}

}
