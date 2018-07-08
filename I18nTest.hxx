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

#include "I18n.h"
#include "MyString.h"


class FormatIntegerTest : public CxxTest::TestSuite {
public:
	
	void testEnglishUsPositive() {
		String output;
		i18n::formatInteger(123456789, &output);
		TS_ASSERT_SAME_DATA(_T("123,456,789"), (LPCTSTR) output, 12 * sizeof(TCHAR));
	}
	
	void testEnglishUsNegative() {
		String output;
		i18n::formatInteger(-123456789, &output);
		TS_ASSERT_SAME_DATA(_T("-123,456,789"), (LPCTSTR) output, 13 * sizeof(TCHAR));
	}
};


class ParseNumberGroupingStringTest : public CxxTest::TestSuite {
public:
	
	void testEmpty() {
		check(_T(""), 0, _T("123456789"));
	}
	
	void testOneGroup() {
		check(_T("3"), 30, _T("123456,789"));
	}
	
	void testRepeatedGroup() {
		check(_T("3;0"), 3, _T("123,456,789"));
	}
	
	void testTwoGroupsNotRepeated() {
		check(_T("3;2"), 320, _T("1234,56,789"));
	}
	
	void testTwoGroupsRepeated() {
		check(_T("3;2;0"), 32, _T("12,34,56,789"));
	}
	
private:
	
	void check(LPCTSTR input, int expected_grouping, LPCTSTR expected_formatted_number) {
		// Check parseNumberGroupingString().
		TS_ASSERT_EQUALS(expected_grouping, i18n::parseNumberGroupingString(input));
		
		// Check the result of GetNumberFormat() with the grouping.
		NUMBERFMT format;
		format.NumDigits = 0;
		format.LeadingZero = TRUE;
		format.lpDecimalSep = _T(".");  // unused
		format.Grouping = expected_grouping;
		format.lpThousandSep = _T(",");
		format.NegativeOrder = 1;
		
		TCHAR formatted_number[30];
		GetNumberFormat(
			LOCALE_USER_DEFAULT, 0,
			_T("123456789"), &format,
			formatted_number, arrayLength(formatted_number));
		TS_ASSERT_SAME_DATA(
			expected_formatted_number,
			formatted_number,
			lstrlen(expected_formatted_number) * sizeof(TCHAR));
	}
};
