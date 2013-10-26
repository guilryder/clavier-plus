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
	
	
	void testGetRightVkFromLeft() {
		TS_ASSERT_EQUALS(VK_RWIN, getRightVkFromLeft(VK_LWIN));
		TS_ASSERT_EQUALS(VK_RCONTROL, getRightVkFromLeft(VK_LCONTROL));
		TS_ASSERT_EQUALS(VK_RSHIFT, getRightVkFromLeft(VK_LSHIFT));
		TS_ASSERT_EQUALS(VK_RMENU, getRightVkFromLeft(VK_LMENU));
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
