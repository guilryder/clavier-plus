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

class ShortcutTest : public CxxTest::TestSuite {
private:
	
	Shortcut* m_shortcut;
	
public:
	
	void setUp() {
		Keystroke ks;
		ks.m_vk = 'A';
		m_shortcut = new Shortcut(ks);
	}
	
	void tearDown() {
		delete m_shortcut;
	}
	
	
	void testSetMatchingLevel_getMatchingLevel() {
		m_shortcut->setMatchingLevel(3);
		TS_ASSERT_EQUALS(3, m_shortcut->getMatchingLevel());
	}
	
	
	void testMatchProgram_noProgramsMismatch() {
		m_shortcut->m_sPrograms = _T("");
		TS_ASSERT_EQUALS(false, m_shortcut->matchProgram(NULL, _T("")));
	}
	
	
	void testMatchProgram_processNameOnly_match() {
		m_shortcut->m_sPrograms = _T("first.exe;test.exe;last.exe");
		TS_ASSERT_EQUALS(true, m_shortcut->matchProgram(_T("test.exe"), _T("")));
	}
	
	void testMatchProgram_processNameOnly_mismatch() {
		m_shortcut->m_sPrograms = _T("first.exe;other.exe;last.exe");
		TS_ASSERT_EQUALS(false, m_shortcut->matchProgram(_T("test.exe"), _T("")));
	}
	
	void testMatchProgram_processNameOnly_unknownProcessNameMatches() {
		m_shortcut->m_sPrograms = _T("first.exe;test.exe;last.exe");
		TS_ASSERT_EQUALS(true, m_shortcut->matchProgram(NULL, _T("Title")));
	}
	
	void testMatchProgram_processNameOnly_caseDoesNotMatter() {
		m_shortcut->m_sPrograms = _T("TeST.exe");
		TS_ASSERT_EQUALS(true, m_shortcut->matchProgram(_T("TeST.exe"), _T("")));
		TS_ASSERT_EQUALS(true, m_shortcut->matchProgram(_T("tESt.exe"), _T("")));
	}
	
	
	void testMatchProgram_windowTitleOnlyNoRegexp_match() {
		m_shortcut->m_sPrograms = _T("first.exe:Blah;:Title;last.exe:Another");
		TS_ASSERT_EQUALS(true, m_shortcut->matchProgram(NULL, _T("Title")));
	}
	
	void testMatchProgram_windowTitleOnlyNoRegexp_mismatch() {
		m_shortcut->m_sPrograms = _T("first.exe:Blah;:Other;last.exe:Another");
		TS_ASSERT_EQUALS(false, m_shortcut->matchProgram(NULL, _T("Title")));
	}
	
	void testMatchProgram_windowTitleOnlyRegexp_match() {
		m_shortcut->m_sPrograms = _T("first.exe:Blah;:*tl?;last.exe:Another");
		TS_ASSERT_EQUALS(true, m_shortcut->matchProgram(NULL, _T("Title")));
	}
	
	void testMatchProgram_windowTitleOnlyRegexp_mismatch() {
		m_shortcut->m_sPrograms = _T("first.exe:Blah;:*test*;last.exe:Another");
		TS_ASSERT_EQUALS(false, m_shortcut->matchProgram(NULL, _T("other")));
	}
	
	void testMatchProgram_windowTitleOnlyRegexp_emptyTitleMismatches() {
		m_shortcut->m_sPrograms = _T("first.exe:Blah;test.exe:Title;last.exe:Another");
		TS_ASSERT_EQUALS(false, m_shortcut->matchProgram(NULL, _T("")));
	}
	
	void testMatchProgram_windowTitleOnly_caseMatters() {
		m_shortcut->m_sPrograms = _T(":Title");
		TS_ASSERT_EQUALS(true, m_shortcut->matchProgram(NULL, _T("Title")));
		TS_ASSERT_EQUALS(false, m_shortcut->matchProgram(NULL, _T("title")));
	}
	
	
	void testMatchProgram_processAndTitle_match() {
		m_shortcut->m_sPrograms = _T("first.exe;test.exe:Title;last.exe");
		TS_ASSERT_EQUALS(true, m_shortcut->matchProgram(_T("test.exe"), _T("Title")));
		TS_ASSERT_EQUALS(true, m_shortcut->matchProgram(NULL, _T("Title")));
	}
	
	void testMatchProgram_processAndTitle_mismatch() {
		m_shortcut->m_sPrograms = _T("first.exe;test.exe:Title;last.exe");
		TS_ASSERT_EQUALS(false, m_shortcut->matchProgram(_T("other.exe"), _T("Title")));
		TS_ASSERT_EQUALS(false, m_shortcut->matchProgram(_T("test.exe"), _T("")));
		TS_ASSERT_EQUALS(false, m_shortcut->matchProgram(_T("test.exe"), _T("Other")));
	}
	
	void testMatchProgram_processAndTitle_lastProgramMatches() {
		m_shortcut->m_sPrograms = _T("first.exe;test.exe:Title");
		TS_ASSERT_EQUALS(true, m_shortcut->matchProgram(_T("test.exe"), _T("Title")));
	}
};
