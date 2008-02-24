/* Generated file, do not edit */

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
//
// Testing template for CxxUnit entry point, instantiated only once.

#ifndef CXXTEST_RUNNING
#define CXXTEST_RUNNING
#endif

#include "stdafx.h"
#include <cxxtest/TestListener.h>
#include <cxxtest/TestTracker.h>
#include <cxxtest/TestRunner.h>
#include <cxxtest/RealDescriptions.h>

#include <cxxtest/ParenPrinter.h>

#include "../App.h"

// Entry point
int main() {
	testing::initialize();
	testing::MemoryLeakChecker memory_leak_checker;
	app::initialize();
	const int exit_code = CxxTest::ParenPrinter().run();
	app::terminate();
	return exit_code;
}

#include <cxxtest/Root.cpp>
