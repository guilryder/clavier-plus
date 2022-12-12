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

#include "CppUnitTest.h"
#include "../MyString.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace testing {

// Normalizes a <0 / =0 / >0 comparison result into -1 / 0 /+1.
int normalizeCompareResult(int result);

// Retrieves the path of the test project directory.
// Buffer size: MAX_PATH.
void getProjectDir(LPTSTR path);

}  // namespace testing


namespace Microsoft::VisualStudio::CppUnitTestFramework {

template<> inline std::wstring ToString<String>(const String& s) { RETURN_WIDE_STRING(s); }

template<> inline std::wstring ToString<IShellLink>(IShellLink* t) {
	RETURN_WIDE_STRING(static_cast<const void*>(t));
}

}  // Microsoft::VisualStudio::CppUnitTestFramework
