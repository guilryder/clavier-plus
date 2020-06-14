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
#include "../Keystroke.h"

namespace KeystrokeTest {

TEST_CLASS(KeystrokeCompareTest) {
public:
	
	TEST_METHOD_INITIALIZE(setUp) {
		m_keystroke1 = new Keystroke();
		m_keystroke2 = new Keystroke();
	}
	
	TEST_METHOD_CLEANUP(tearDown) {
		delete m_keystroke1;
		delete m_keystroke2;
	}
	
	TEST_METHOD(Equal) {
		setKeystroke(m_keystroke1, 'A', false, 0);
		setKeystroke(m_keystroke2, 'A', false, 0);
		checkCompare(0);
	}
	
	TEST_METHOD(NoModCode) {
		setKeystroke(m_keystroke1, 'A', false, 0);
		setKeystroke(m_keystroke2, 'B', false, 0);
		checkCompare(-1);
	}
	
	TEST_METHOD(SameModCode) {
		setKeystroke(m_keystroke1, 'A', false, MOD_CONTROL | MOD_SHIFT);
		setKeystroke(m_keystroke2, 'B', false, MOD_CONTROL | MOD_SHIFT);
		checkCompare(-1);
	}
	
	TEST_METHOD(WinBeforeControl) {
		setKeystroke(m_keystroke1, 'A', false, MOD_WIN);
		setKeystroke(m_keystroke2, 'A', false, MOD_CONTROL);
		checkCompare(-1);
	}
	
	TEST_METHOD(ControlBeforeShift) {
		setKeystroke(m_keystroke1, 'A', false, MOD_CONTROL);
		setKeystroke(m_keystroke2, 'A', false, MOD_SHIFT);
		checkCompare(-1);
	}
	
	TEST_METHOD(ShiftBeforeAlt) {
		setKeystroke(m_keystroke1, 'A', false, MOD_SHIFT);
		setKeystroke(m_keystroke2, 'A', false, MOD_ALT);
		checkCompare(-1);
	}
	
	TEST_METHOD(WinControlBeforeWinControlShift) {
		setKeystroke(m_keystroke1, 'A', false, MOD_WIN | MOD_CONTROL);
		setKeystroke(m_keystroke2, 'A', false, MOD_WIN | MOD_CONTROL | MOD_SHIFT);
		checkCompare(-1);
	}
	
	TEST_METHOD(WinControlShiftBeforeWinControlAlt) {
		setKeystroke(m_keystroke1, 'A', false, MOD_WIN | MOD_CONTROL | MOD_SHIFT);
		setKeystroke(m_keystroke2, 'A', false, MOD_WIN | MOD_CONTROL | MOD_ALT);
		checkCompare(-1);
	}
	
	TEST_METHOD(NoneBeforeUnsided) {
		setKeystroke(m_keystroke1, 'A', false, 0);
		setKeystroke(m_keystroke2, 'A', false, MOD_ALT);
		checkCompare(-1);
	}
	
	TEST_METHOD(UnsidedBeforeLeft) {
		setKeystroke(m_keystroke1, 'A', false, MOD_ALT);
		setKeystroke(m_keystroke2, 'A', true, MOD_ALT);
		checkCompare(-1);
	}
	
	TEST_METHOD(LeftBeforeRight) {
		setKeystroke(m_keystroke1, 'A', true, MOD_ALT);
		setKeystroke(m_keystroke2, 'A', true, MOD_ALT << kRightModCodeOffset);
		checkCompare(-1);
	}
	
	TEST_METHOD(SortedList) {
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
		
		Assert::AreEqual(static_cast<size_t>(keystroke - keystrokes), arrayLength(keystrokes));
		
		for (int i = 0; i < arrayLength(keystrokes); i++) {
			for (int j = 0; j < arrayLength(keystrokes); j++) {
				checkCompare(i - j, keystrokes[i], keystrokes[j]);
			}
		}
	}
	
private:
	
	Keystroke* m_keystroke1;
	Keystroke* m_keystroke2;
	
	void checkCompare(int expected_result) {
		checkCompare(+expected_result, *m_keystroke1, *m_keystroke2);
		checkCompare(-expected_result, *m_keystroke2, *m_keystroke1);
	}
	
	void checkCompare(int expected_result, const Keystroke& keystroke1, const Keystroke& keystroke2) {
		expected_result = testing::normalizeCompareResult(expected_result);
		int actual_result = testing::normalizeCompareResult(Keystroke::compare(keystroke1, keystroke2));
		
		// Generate a nice test failure message.
		TCHAR message[50 + bufHotKey * 2];
		TCHAR name1[bufHotKey], name2[bufHotKey];
		keystroke1.getDisplayName(name1);
		keystroke2.getDisplayName(name2);
		wsprintf(message, _T("Comparing %ws and %ws -- expected %d"),
			name1, name2, expected_result);
		
		Assert::AreEqual(expected_result, actual_result, message);
	}
	
	void setKeystroke(Keystroke* keystroke, BYTE vk, bool sided, int sided_mod_code) {
		keystroke->m_vk = vk;
		keystroke->m_sided = sided;
		keystroke->m_sided_mod_code = sided_mod_code;
	}
};

TEST_CLASS(SpecialKeyTest) {
public:
	
	TEST_METHOD(VkRight) {
		for (int i = 0; i < arrayLength(e_special_keys); i++) {
			const SpecialKey& special_key = e_special_keys[i];
			Assert::AreEqual(static_cast<BYTE>(special_key.vk_left + 1), special_key.vk_right);
		}
	}
};

}
