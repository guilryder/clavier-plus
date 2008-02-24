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

#include "stdafx.h"
#include "Testing.h"

#include <eh.h>

namespace testing {

void initialize() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	for (int i = 0; i < _CRT_ERRCNT; i++) {
		_CrtSetReportMode(i, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
		_CrtSetReportFile(i, _CRTDBG_FILE_STDERR);
	}
	
	SeException::initialize();
}

void SeException::initialize() {
	_set_se_translator(translateSehToCpp);
}

void SeException::translateSehToCpp(UINT code, _EXCEPTION_POINTERS* MY_UNUSED(details)) {
	throw SeException(code);
}


bool MemoryLeakChecker::setUp() {
	_CrtMemCheckpoint(&m_before_mem_state);
	return true;
}

bool MemoryLeakChecker::tearDown() {
	_CrtMemState after_mem_state, diff_mem_state;
	_CrtMemCheckpoint(&after_mem_state);
	if (_CrtMemDifference(&diff_mem_state, &m_before_mem_state, &after_mem_state)) {
		_CrtMemDumpAllObjectsSince(&m_before_mem_state);
		return false;
	}
	return true;
}

}  // testing namespace
