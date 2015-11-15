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

#include "Shortcut.h"


static const Keystroke s_ks_empty;
static const String* NO_PROGRAMS = NULL;


class ShortcutGetProgramsTest : public CxxTest::TestSuite {
private:
	
	Shortcut m_shortcut;
	
public:
	
	ShortcutGetProgramsTest() : m_shortcut(s_ks_empty) {}
	
	void testEmpty() {
		TS_ASSERT_EQUALS(NO_PROGRAMS, m_shortcut.getPrograms());
	}
	
	void testOnlySeparators() {
		m_shortcut.m_sPrograms = _T(";");
		String* actual = m_shortcut.getPrograms();
		TS_ASSERT_DIFFERS(NO_PROGRAMS, actual);
		TS_ASSERT_EQUALS(0, lstrcmp(actual[0], _T("")));
		delete [] actual;
	}
	
	void testOne() {
		m_shortcut.m_sPrograms = _T("prog1");
		String* actual = m_shortcut.getPrograms();
		TS_ASSERT_DIFFERS(NO_PROGRAMS, actual);
		TS_ASSERT_EQUALS(0, lstrcmp(actual[0], _T("prog1")));
		TS_ASSERT_EQUALS(0, lstrcmp(actual[1], _T("")));
		delete [] actual;
	}
	
	void testSeveral() {
		m_shortcut.m_sPrograms = _T("prog1;prog2");
		String* actual = m_shortcut.getPrograms();
		TS_ASSERT_DIFFERS(NO_PROGRAMS, actual);
		TS_ASSERT_EQUALS(0, lstrcmp(actual[0], _T("prog1")));
		TS_ASSERT_EQUALS(0, lstrcmp(actual[1], _T("prog2")));
		TS_ASSERT_EQUALS(0, lstrcmp(actual[2], _T("")));
		delete [] actual;
	}
	
	void testSkipsEmpty() {
		m_shortcut.m_sPrograms = _T(";prog1;;prog2;");
		String* actual = m_shortcut.getPrograms();
		TS_ASSERT_DIFFERS(NO_PROGRAMS, actual);
		TS_ASSERT_EQUALS(0, lstrcmp(actual[0], _T("prog1")));
		TS_ASSERT_EQUALS(0, lstrcmp(actual[1], _T("prog2")));
		TS_ASSERT_EQUALS(0, lstrcmp(actual[2], _T("")));
		delete [] actual;
	}
};


class ShortcutCleanProgramsTest : public CxxTest::TestSuite {
private:
	
	Shortcut m_shortcut;
	
	void check(LPCTSTR programsBefore, LPCTSTR programsExpectedAfter) {
		m_shortcut.m_sPrograms = programsBefore;
		m_shortcut.cleanPrograms();
		TSM_ASSERT_EQUALS(m_shortcut.m_sPrograms,
		                  0, lstrcmp(m_shortcut.m_sPrograms, programsExpectedAfter));
	}
	
public:
	
	ShortcutCleanProgramsTest() : m_shortcut(s_ks_empty) {}
	
	void testEmpty() {
		check(_T(""), _T(""));
	}
	
	void testOnlySeparators() {
		check(_T(";"), _T(""));
		check(_T(";;"), _T(""));
	}
	
	void testOne() {
		check(_T("prog1"), _T("prog1"));
	}
	
	void testSeveral() {
		check(_T("prog1;prog2"), _T("prog1;prog2"));
	}
	
	void testSkipsEmpty() {
		check(_T(";prog1;;prog2;"), _T("prog1;prog2"));
	}
	
	void testIgnoresCase() {
		check(_T("prog1;PrOg1;PROG1"), _T("prog1"));
	}
	
	void testDropsDuplicates() {
		check(_T("prog1;prog2;prog1"), _T("prog1;prog2"));
		check(_T("prog1;prog1;prog1"), _T("prog1"));
		check(_T("prog1;prog2;prog1;prog3;prog1;prog2"), _T("prog1;prog2;prog3"));
		check(_T("prog1;prog2;prog3;prog2"), _T("prog1;prog2;prog3"));
	}
};


class ShortcutMatchTest : public CxxTest::TestSuite {
private:
	
	Keystroke m_ks;
	
public:
	
	void setUp() {
		m_ks.m_vk = 'A';
		m_ks.m_sided_mod_code = MOD_CONTROL;
	}
	
	void testBasic() {
		Shortcut shortcut(m_ks);
		
		Keystroke ks_matching;
		ks_matching.m_vk = 'A';
		ks_matching.m_sided_mod_code = MOD_CONTROL;
		ks_matching.m_sided = true;
		
		Keystroke ks_different(m_ks);
		ks_different.m_vk = 'B';
		
		TS_ASSERT_EQUALS(true, shortcut.match(m_ks, NULL));
		TS_ASSERT_EQUALS(true, shortcut.match(ks_matching, NULL));
		TS_ASSERT_EQUALS(true, shortcut.match(m_ks, _T("prog")));
		TS_ASSERT_EQUALS(false, shortcut.match(ks_different, NULL));
	}
	
	void testProgramsOnlyTrue() {
		testProgramsOnly(true);
	}
	
	void testProgramsOnlyFalse() {
		testProgramsOnly(false);
	}
	
	void testProgramsOnly(bool programsOnly) {
		Shortcut shortcut(m_ks);
		shortcut.m_bProgramsOnly = programsOnly;
		shortcut.m_sPrograms = _T("prog1;prog2;prog3");
		
		TS_ASSERT_EQUALS(!programsOnly, shortcut.match(m_ks, NULL));
		TS_ASSERT_EQUALS(programsOnly, shortcut.match(m_ks, _T("prog1")));
		TS_ASSERT_EQUALS(programsOnly, shortcut.match(m_ks, _T("PrOg1")));
		TS_ASSERT_EQUALS(programsOnly, shortcut.match(m_ks, _T("PROG1")));
		TS_ASSERT_EQUALS(programsOnly, shortcut.match(m_ks, _T("prog2")));
		TS_ASSERT_EQUALS(programsOnly, shortcut.match(m_ks, _T("prog3")));
		TS_ASSERT_EQUALS(!programsOnly, shortcut.match(m_ks, _T("prog1.exe")));
		TS_ASSERT_EQUALS(!programsOnly, shortcut.match(m_ks, _T("pro")));
		TS_ASSERT_EQUALS(!programsOnly, shortcut.match(m_ks, _T("ro")));
		TS_ASSERT_EQUALS(!programsOnly, shortcut.match(m_ks, _T("rog2")));
		TS_ASSERT_EQUALS(!programsOnly, shortcut.match(m_ks, _T("other")));
	}
};


class ShortcutCompareTest : public CxxTest::TestSuite {
private:
	
	Shortcut* m_shortcut1;
	Shortcut* m_shortcut2;
	
	void checkCompare(int sort_column, int expected_result) {
		Shortcut::s_iSortColumn = sort_column;
		TS_ASSERT_EQUALS(+expected_result,
			testing::normalizeCompareResult(Shortcut::compare(m_shortcut1, m_shortcut2, 0)));
		TS_ASSERT_EQUALS(-expected_result,
				testing::normalizeCompareResult(Shortcut::compare(m_shortcut2, m_shortcut1, 0)));
	}
	
public:
	
	void setUp() {
		Keystroke ks;
		ks.m_vk = 'A';
		m_shortcut1 = new Shortcut(ks);
		m_shortcut2 = new Shortcut(ks);
	}
	
	void tearDown() {
		delete m_shortcut1;
		delete m_shortcut2;
	}
	
	
	void testContents_bothAreCommands() {
		m_shortcut1->m_bCommand = m_shortcut2->m_bCommand = true;
		m_shortcut1->m_sCommand = _T("AZZZZZ");
		m_shortcut2->m_sCommand = _T("Baa");
		checkCompare(colContents, -1);
	}
	
	void testContents_bothAreTexts() {
		m_shortcut1->m_bCommand = m_shortcut2->m_bCommand = false;
		m_shortcut1->m_sText = _T("Baa");
		m_shortcut2->m_sText = _T("AZZZZZ");
		checkCompare(colContents, +1);
	}
	
	void testContents_commandAndText() {
		m_shortcut1->m_bCommand = false;
		m_shortcut2->m_bCommand = true;
		checkCompare(colContents, -1);
	}
	
	
	void testKeystroke_equal() {
		checkCompare(colKeystroke, 0);
	}
	
	void testKeystroke_noModCode() {
		m_shortcut1->m_vk = 'A';
		m_shortcut2->m_vk = 'B';
		checkCompare(colKeystroke, -1);
	}
	
	void testKeystroke_sameModCode() {
		m_shortcut1->m_vk = 'A';
		m_shortcut2->m_vk = 'B';
		m_shortcut1->m_sided_mod_code = m_shortcut2->m_sided_mod_code
				= MOD_ALT | MOD_SHIFT | MOD_CONTROL | MOD_WIN;
		checkCompare(colKeystroke, -1);
	}
	
	void testKeystroke_winControlShiftBeforeWinControlAlt() {
		m_shortcut1->m_sided_mod_code = MOD_WIN | MOD_CONTROL | MOD_SHIFT;
		m_shortcut2->m_sided_mod_code = MOD_WIN | MOD_CONTROL | MOD_ALT;
		checkCompare(colKeystroke, -1);
	}
	
	
	void testCond_empty() {
		checkCompare(colCond, 0);
	}
	
	void testCond_equal() {
		m_shortcut1->m_aCond[condTypeShiftLock] = m_shortcut2->m_aCond[condTypeShiftLock] = +1;
		m_shortcut1->m_sPrograms = m_shortcut2->m_sPrograms = _T("same");
		checkCompare(colCond, 0);
	}
	
	void testCond_differentCondition() {
		m_shortcut1->m_aCond[condTypeShiftLock] = +1;
		m_shortcut2->m_aCond[condTypeNumLock] = -1;
		checkCompare(colCond, -1);
	}
	
	void testCond_differentPrograms() {
		m_shortcut1->m_sPrograms = _T("aaa");
		m_shortcut2->m_sPrograms = _T("bbb");
		checkCompare(colCond, -1);
	}
	
	
	void testDescription_empty() {
		checkCompare(colDescription, 0);
	}
	
	void testDescription_equal() {
		m_shortcut1->m_sDescription = m_shortcut2->m_sDescription = _T("same");
		checkCompare(colDescription, 0);
	}
	
	void testDescription_different() {
		m_shortcut1->m_sDescription = _T("Aaaa");
		m_shortcut2->m_sDescription = _T("Bbbb");
		checkCompare(colDescription, -1);
	}
	
	void testDescription_ignoreCase() {
		m_shortcut1->m_sDescription = _T("AAAA");
		m_shortcut2->m_sDescription = _T("aaaa");
		checkCompare(colDescription, 0);
	}
};
