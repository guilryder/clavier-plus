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

#ifndef TESTING
#error TESTING must be defined.
#endif // !TESTING

#ifndef _DEBUG
#error TESTING mode is only available in _DEBUG mode.
#endif // !_DEBUG

#define CXXTEST_HAVE_EH

#include <cxxtest/GlobalFixture.h>
#include <cxxtest/TestSuite.h>

namespace testing {

// Initializes the testing framework.
// Enables debugging to stdout, translates SEH exceptions to SeException.
void initialize();

// A wrapper for SEH exceptions. Allows to catch SEH exceptions with C++ try/catch statements.
// Can be used with TS_ASSERT_THROWS to catch SEH exceptions, such as memory exceptions.
class SeException {
public:
	
	// Sets up the translator that throws SeException exceptions from SEH exceptions.
	static void initialize();
	
	// Builds an exception wrapper for a given SEH code.
	SeException(UINT code) : m_code(code) {
	}
	
	UINT getCode() {
		return m_code;
	}
	
private:
	UINT m_code;
	
	// Callback for _set_se_translator() that raises a C++ SeException from a SEH exception.
	static void translateSehToCpp(UINT code, _EXCEPTION_POINTERS* details);
};


// Global fixture ensuring that no memory leak occured between just before setUp() and just after
// tearDown(). All tests are supposed to keep the memory state unchanged. In particular, a test
// should not allocate or free a member variable of the test suite.
class MemoryLeakChecker : public CxxTest::GlobalFixture {
private:
	_CrtMemState m_before_mem_state;  // Memory state before executing the test
	
public:
	
	// Saves the current memory state.
	bool setUp();
	
	// Ensures that the current memory state is the same than the one saved previously by setUp().
	// If not, prints the memory leaks and fails the test.
	bool tearDown();
};

}  // testing namespace
