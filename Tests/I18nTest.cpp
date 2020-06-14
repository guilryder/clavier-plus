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
#include "../I18n.h"
#include "../MyString.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace I18nTest {

TEST_CLASS(FormatIntegerTest) {
public:
	
	TEST_METHOD(EnglishUsPositive) {
		String output;
		i18n::formatInteger(123456789, &output);
		Assert::AreEqual(_T("123,456,789"), output);
	}
	
	TEST_METHOD(EnglishUsNegative) {
		String output;
		i18n::formatInteger(-123456789, &output);
		Assert::AreEqual(_T("-123,456,789"), output);
	}
};


TEST_CLASS(ParseNumberGroupingStringTest) {
public:
	
	TEST_METHOD(Empty) {
		check(_T(""), 0, _T("123456789"));
	}
	
	TEST_METHOD(OneGroup) {
		check(_T("3"), 30, _T("123456,789"));
	}
	
	TEST_METHOD(RepeatedGroup) {
		check(_T("3;0"), 3, _T("123,456,789"));
	}
	
	TEST_METHOD(TwoGroupsNotRepeated) {
		check(_T("3;2"), 320, _T("1234,56,789"));
	}
	
	TEST_METHOD(TwoGroupsRepeated) {
		check(_T("3;2;0"), 32, _T("12,34,56,789"));
	}
	
private:
	
	void check(LPCTSTR input, int expected_grouping, LPCTSTR expected_formatted_number) {
		// Check parseNumberGroupingString().
		Assert::AreEqual(expected_grouping, i18n::parseNumberGroupingString(input));
		
		// Check the result of GetNumberFormat() with the grouping.
		NUMBERFMT format;
		format.NumDigits = 0;
		format.LeadingZero = true;
		format.lpDecimalSep = _T(".");  // unused
		format.Grouping = expected_grouping;
		format.lpThousandSep = _T(",");
		format.NegativeOrder = 1;
		
		TCHAR formatted_number[30];
		GetNumberFormat(
			LOCALE_USER_DEFAULT, 0,
			_T("123456789"), &format,
			formatted_number, arrayLength(formatted_number));
		Assert::AreEqual(expected_formatted_number, formatted_number);
	}
};

}
