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

#include "Global.h"

class MatchWildcardsTest : public CxxTest::TestSuite {
public:
	
	void testEmpty_matches_empty() {
		TS_ASSERT_EQUALS(true, matchWildcards(_T(""), _T("")));
	}
	
	void testEmpty_mismatches_notEmpty() {
		TS_ASSERT_EQUALS(false, matchWildcards(_T(""), _T("Subject")));
	}
	
	void testSameString_matches() {
		TS_ASSERT_EQUALS(true, matchWildcards(_T("Test"), _T("Test")));
	}
	
	void testCaseMatters() {
		TS_ASSERT_EQUALS(false, matchWildcards(_T("test"), _T("Test")));
		TS_ASSERT_EQUALS(false, matchWildcards(_T("test"), _T("TEST")));
	}
	
	void testQuestionMark_one() {
		TS_ASSERT_EQUALS(true, matchWildcards(_T("te?t"), _T("test")));
	}
	
	void testQuestionMark_two() {
		TS_ASSERT_EQUALS(true, matchWildcards(_T("?e?t"), _T("test")));
	}
	
	void testQuestionMark_needsOtherCharacterSame() {
		TS_ASSERT_EQUALS(false, matchWildcards(_T("te?t"), _T("xxxx")));
	}
	
	void testQuestionMark_needsSameLength() {
		TS_ASSERT_EQUALS(true, matchWildcards(_T("????"), _T("test")));
		TS_ASSERT_EQUALS(false, matchWildcards(_T("?????"), _T("test")));
		TS_ASSERT_EQUALS(false, matchWildcards(_T("???"), _T("test")));
	}
	
	void testStart_atTheBeginning() {
		TS_ASSERT_EQUALS(true, matchWildcards(_T("*end"), _T("blah end")));
		TS_ASSERT_EQUALS(false, matchWildcards(_T("*end"), _T("blah Xnd")));
	}
	
	void testStar_atTheEnd() {
		TS_ASSERT_EQUALS(true, matchWildcards(_T("beginning*"), _T("beginning blah")));
		TS_ASSERT_EQUALS(false, matchWildcards(_T("beginning*"), _T("beginninX blah")));
	}
	
	void testStar_beginningAndEnd() {
		TS_ASSERT_EQUALS(true, matchWildcards(_T("*middle*"), _T("first middle last")));
		TS_ASSERT_EQUALS(false, matchWildcards(_T("*middle*"), _T("first Xiddle last")));
		TS_ASSERT_EQUALS(false, matchWildcards(_T("*middle*"), _T("first middlX last")));
	}
	
	void testStar_middle() {
		TS_ASSERT_EQUALS(true, matchWildcards(_T("first*last"), _T("first blah last")));
		TS_ASSERT_EQUALS(false, matchWildcards(_T("first*last"), _T("firsX blah last")));
		TS_ASSERT_EQUALS(false, matchWildcards(_T("first*last"), _T("first blah Xast")));
	}
	
	void testPatternEnd_noWildcards() {
		const LPCTSTR pattern = _T("testXXX");
		TS_ASSERT_EQUALS(true, matchWildcards(pattern, _T("test"), pattern + 4));
		TS_ASSERT_EQUALS(false, matchWildcards(pattern, _T("test"), pattern + 3));
	}
	
	void testPatternEnd_withStar() {
		const LPCTSTR pattern = _T("te*tXXX");
		TS_ASSERT_EQUALS(true, matchWildcards(pattern, _T("tessst"), pattern + 4));
		TS_ASSERT_EQUALS(false, matchWildcards(pattern, _T("tessst"), pattern + 5));
	}
	
	void testPatternEnd_withStarAndQuestionMark() {
		const LPCTSTR pattern = _T("*tl?XXX");
		TS_ASSERT_EQUALS(true, matchWildcards(pattern, _T("Title"), pattern + 4));
	}
};
