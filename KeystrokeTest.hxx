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

#include "Keystroke.h"


class KeystrokeCompareTest : public CxxTest::TestSuite {
private:
	
	Keystroke* m_keystroke1;
	Keystroke* m_keystroke2;
	
	void checkCompare(int expected_result) {
		checkCompare(+expected_result, *m_keystroke1, *m_keystroke2);
		checkCompare(-expected_result, *m_keystroke2, *m_keystroke1);
	}
	
	void checkCompare(int expected_result, const Keystroke& keystroke1, const Keystroke& keystroke2) {
		expected_result = testing::normalizeCompareResult(expected_result);
		
		char message[50 + bufHotKey * 2];
		TCHAR name1[bufHotKey], name2[bufHotKey];
		keystroke1.getKeyName(name1);
		keystroke2.getKeyName(name2);
		wsprintfA(message, "Comparing %ws and %ws -- expected %d",
				name1, name2, expected_result);
		
		TSM_ASSERT_EQUALS(message, expected_result,
			testing::normalizeCompareResult(Keystroke::compare(keystroke1, keystroke2)));
	}
	
	void setKeystroke(Keystroke* keystroke, BYTE vk, bool sided, int sided_mod_code) {
		keystroke->m_vk = vk;
		keystroke->m_sided = sided;
		keystroke->m_sided_mod_code = sided_mod_code;
	}
	
public:
	
	void setUp() {
		m_keystroke1 = new Keystroke();
		m_keystroke2 = new Keystroke();
	}
	
	void tearDown() {
		delete m_keystroke1;
		delete m_keystroke2;
	}
	
	void testEqual() {
		setKeystroke(m_keystroke1, 'A', false, 0);
		setKeystroke(m_keystroke2, 'A', false, 0);
		checkCompare(0);
	}
	
	void testNoModCode() {
		setKeystroke(m_keystroke1, 'A', false, 0);
		setKeystroke(m_keystroke2, 'B', false, 0);
		checkCompare(-1);
	}
	
	void testSameModCode() {
		setKeystroke(m_keystroke1, 'A', false, MOD_CONTROL | MOD_SHIFT);
		setKeystroke(m_keystroke2, 'B', false, MOD_CONTROL | MOD_SHIFT);
		checkCompare(-1);
	}
	
	void testWinBeforeControl() {
		setKeystroke(m_keystroke1, 'A', false, MOD_WIN);
		setKeystroke(m_keystroke2, 'A', false, MOD_CONTROL);
		checkCompare(-1);
	}
	
	void testControlBeforeShift() {
		setKeystroke(m_keystroke1, 'A', false, MOD_CONTROL);
		setKeystroke(m_keystroke2, 'A', false, MOD_SHIFT);
		checkCompare(-1);
	}
	
	void testShiftBeforeAlt() {
		setKeystroke(m_keystroke1, 'A', false, MOD_SHIFT);
		setKeystroke(m_keystroke2, 'A', false, MOD_ALT);
		checkCompare(-1);
	}
	
	void testWinControlBeforeWinControlShift() {
		setKeystroke(m_keystroke1, 'A', false, MOD_WIN | MOD_CONTROL);
		setKeystroke(m_keystroke2, 'A', false, MOD_WIN | MOD_CONTROL | MOD_SHIFT);
		checkCompare(-1);
	}
	
	void testWinControlShiftBeforeWinControlAlt() {
		setKeystroke(m_keystroke1, 'A', false, MOD_WIN | MOD_CONTROL | MOD_SHIFT);
		setKeystroke(m_keystroke2, 'A', false, MOD_WIN | MOD_CONTROL | MOD_ALT);
		checkCompare(-1);
	}
	
	void testNoneBeforeUnsided() {
		setKeystroke(m_keystroke1, 'A', false, 0);
		setKeystroke(m_keystroke2, 'A', false, MOD_ALT);
		checkCompare(-1);
	}
	
	void testUnsidedBeforeLeft() {
		setKeystroke(m_keystroke1, 'A', false, MOD_ALT);
		setKeystroke(m_keystroke2, 'A', true, MOD_ALT);
		checkCompare(-1);
	}
	
	void testLeftBeforeRight() {
		setKeystroke(m_keystroke1, 'A', true, MOD_ALT);
		setKeystroke(m_keystroke2, 'A', true, MOD_ALT << kRightModCodeOffset);
		checkCompare(-1);
	}
	
	void testSortedList() {
		Keystroke keystrokes[21];
		Keystroke* keystroke = keystrokes;
		
		setKeystroke(keystroke++, 'W', false, 0);
		setKeystroke(keystroke++, 'W', false, MOD_WIN);
		
		setKeystroke(keystroke++, 'X', false, 0);
		
		setKeystroke(keystroke++, 'X', false, MOD_WIN);
		setKeystroke(keystroke++, 'X', false, MOD_WIN | MOD_CONTROL);
		setKeystroke(keystroke++, 'X', false, MOD_WIN | MOD_CONTROL | MOD_SHIFT);
		setKeystroke(keystroke++, 'X', false, MOD_WIN | MOD_SHIFT);
		setKeystroke(keystroke++, 'X', true, MOD_WIN | MOD_SHIFT);
		setKeystroke(keystroke++, 'X', true, MOD_WIN | (MOD_SHIFT << kRightModCodeOffset));
		
		setKeystroke(keystroke++, 'X', false, MOD_CONTROL);
		setKeystroke(keystroke++, 'X', false, MOD_CONTROL | MOD_SHIFT);
		setKeystroke(keystroke++, 'X', false, MOD_CONTROL | MOD_SHIFT | MOD_ALT);
		setKeystroke(keystroke++, 'X', false, MOD_CONTROL | MOD_ALT);
		setKeystroke(keystroke++, 'X', true, MOD_CONTROL);
		
		setKeystroke(keystroke++, 'X', false, MOD_SHIFT);
		setKeystroke(keystroke++, 'X', true, MOD_SHIFT);
		setKeystroke(keystroke++, 'X', true, MOD_SHIFT << kRightModCodeOffset);
		
		setKeystroke(keystroke++, 'X', false, MOD_ALT);
		setKeystroke(keystroke++, 'X', true, MOD_ALT);
		setKeystroke(keystroke++, 'X', true, MOD_ALT << kRightModCodeOffset);
		
		setKeystroke(keystroke++, 'Y', false, 0);
		
		TS_ASSERT_EQUALS(static_cast<int>(keystroke - keystrokes), arrayLength(keystrokes));
		
		for (int i = 0; i < arrayLength(keystrokes); i++) {
			for (int j = 0; j < arrayLength(keystrokes); j++) {
				checkCompare(i - j, keystrokes[i], keystrokes[j]);
			}
		}
	}
};


class SpecialKeyTest : public CxxTest::TestSuite {
public:
	
	void testVkRight() {
		for (int i = 0; i < arrayLength(e_special_keys); i++) {
			const SpecialKey& special_key = e_special_keys[i];
			TS_ASSERT_EQUALS(static_cast<BYTE>(special_key.vk_left + 1), special_key.vk_right);
		}
	}
};
