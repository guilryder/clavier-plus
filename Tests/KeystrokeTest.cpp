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

void useEnglishKeyNames() {
	i18n::setLanguage(i18n::langEN);
}

void setKeystroke(Keystroke* keystroke, BYTE vk, bool sided = false, DWORD sided_mod_code = 0) {
	keystroke->clear();
	keystroke->m_vk = vk;
	keystroke->m_sided = sided;
	keystroke->m_sided_mod_code = sided_mod_code;
}


// GetDisplayName and ParseDisplayName
TEST_CLASS(DisplayNameTest) {
public:
	
	TEST_CLASS_INITIALIZE(classSetUp) {
		useEnglishKeyNames();
	}
	
	TEST_METHOD(Letter) {
		checkDisplayName(_T("X"), 'X');
	}
	
	TEST_METHOD(NonCharacter) {
		checkDisplayName(_T("Page Up"), VK_PRIOR);
	}
	
	TEST_METHOD(NumericCode) {
		checkDisplayName(_T("#166"), VK_BROWSER_BACK);
	}
	
	TEST_METHOD(SpecialKeys_unsided) {
		checkDisplayName(_T("Win + X"), 'X', /* sided= */ false, MOD_WIN);
		checkDisplayName(_T("Ctrl + X"), 'X', /* sided= */ false, MOD_CONTROL);
		checkDisplayName(_T("Shift + X"), 'X', /* sided= */ false, MOD_SHIFT);
		checkDisplayName(_T("Alt + X"), 'X', /* sided= */ false, MOD_ALT);
		checkDisplayName(
			_T("Win + Ctrl + Shift + Alt + X"), 'X',
			/* sided= */ false,
			MOD_WIN | MOD_CONTROL | MOD_SHIFT | MOD_ALT);
	}
	
	TEST_METHOD(SpecialKeys_sided) {
		checkDisplayName(_T("Win Left + X"), 'X', /* sided= */ true, MOD_WIN);
		checkDisplayName(_T("Ctrl Right + X"), 'X', /* sided= */ true, MOD_CONTROL << kRightModCodeOffset);
		checkDisplayName(_T("Shift Left + X"), 'X', /* sided= */ true, MOD_SHIFT);
		checkDisplayName(_T("Alt Right + X"), 'X', /* sided= */ true, MOD_ALT << kRightModCodeOffset);
		checkDisplayName(
			_T("Win Right + Ctrl Left + Shift Right + Alt Left + X"), 'X',
			/* sided= */ true,
			MOD_CONTROL | MOD_ALT | ((MOD_WIN | MOD_SHIFT) << kRightModCodeOffset));
	}
	
	TEST_METHOD(SpecialKeys_only) {
		checkDisplayName(_T("Ctrl + "), 0, /* sided= */ false, MOD_CONTROL);
		checkDisplayName(
			_T("Win Right + Ctrl Left + Shift Right + Alt Left + "), 0,
			/* sided= */ true,
			MOD_CONTROL | MOD_ALT | ((MOD_WIN | MOD_SHIFT) << kRightModCodeOffset));
	}
	
private:
	
	void checkDisplayName(LPCTSTR display_name, BYTE vk, bool sided = false, DWORD sided_mod_code = 0) {
		Keystroke keystroke;
		setKeystroke(&keystroke, vk, sided, sided_mod_code);
		String keystroke_debug_string = keystroke.debugString();
		
		// Verify getDisplayName().
		TCHAR generated_display_name[bufHotKey];
		keystroke.getDisplayName(generated_display_name);
		Assert::AreEqual(
			display_name, generated_display_name,
			StringPrintf(_T("getDisplayName mismatch for %s"), keystroke_debug_string.getSafe()));
		
		// Verify parseDisplayName().
		Keystroke parsed_keystroke;
		parsed_keystroke.parseDisplayName(String(display_name).get());
		Assert::AreEqual(
			keystroke_debug_string.getSafe(), parsed_keystroke.debugString().getSafe(),
			StringPrintf(_T("parseDisplayName mismatch for %s"), display_name));
	}
};


TEST_CLASS(ParseDisplayNameTest) {
public:
	
	TEST_CLASS_INITIALIZE(classSetUp) {
		useEnglishKeyNames();
	}
	
	TEST_METHOD(KeySeparators_ignoresWhitespace) {
		checkParseDisplayName(_T("Win+Ctrl+X"), 'X', /* sided= */ false, MOD_WIN | MOD_CONTROL);
		checkParseDisplayName(_T("Win  Right  +   Ctrl  Left  +   X"), 'X', /* sided= */ true, (MOD_WIN << kRightModCodeOffset) | MOD_CONTROL);
	}
	
	TEST_METHOD(KeySeparators_plusOptional) {
		checkParseDisplayName(_T("Win X"), 'X', /* sided= */ false, MOD_WIN);
		checkParseDisplayName(_T("Win Right Ctrl Left X"), 'X', /* sided= */ true, (MOD_WIN << kRightModCodeOffset) | MOD_CONTROL);
	}
	
	TEST_METHOD(KeySeparators_atMostOnePlus) {
		checkParseDisplayName(_T("Win ++ X"), 0, /* sided= */ false, MOD_WIN);
		checkParseDisplayName(_T("Win + + X"), 0, /* sided= */ false, MOD_WIN);
	}
	
	TEST_METHOD(SpecialKeys_only) {
		checkParseDisplayName(_T("Ctrl"), 0, /* sided= */ false, MOD_CONTROL);
		checkParseDisplayName(
			_T("Win Right + Ctrl Left + Shift Right + Alt Left"), 0,
			/* sided= */ true,
			MOD_CONTROL | MOD_ALT | ((MOD_WIN | MOD_SHIFT) << kRightModCodeOffset));
	}
	
private:
	
	void checkParseDisplayName(LPCTSTR input, BYTE vk, bool sided = false, DWORD sided_mod_code = 0) {
		Keystroke expected;
		expected.m_vk = vk;
		expected.m_sided = sided;
		expected.m_sided_mod_code = sided_mod_code;
		
		Keystroke actual;
		actual.parseDisplayName(String(input).get());
		Assert::AreEqual(
			expected.debugString().getSafe(), actual.debugString().getSafe(),
			StringPrintf(_T("parseDisplayName mismatch for %s"), input));
	}
};


TEST_CLASS(GetKeyNameTest) {
public:
	
	TEST_CLASS_INITIALIZE(classSetUp) {
		useEnglishKeyNames();
	}
	
	TEST_METHOD(OutOfRange) {
		checkGetKeyName(_T(""), 0);
		checkGetKeyName(_T(""), 1);
		checkGetKeyName(_T(""), 7);
		checkGetKeyName(_T(""), 256);
	}
	
	TEST_METHOD(Reserved) {
		checkGetKeyName(_T(""), 0x0A);
		checkGetKeyName(_T(""), 0x0B);
		checkGetKeyName(_T(""), 0x5E);
		checkGetKeyName(_T(""), VK_LSHIFT);
		checkGetKeyName(_T(""), VK_RSHIFT);
		checkGetKeyName(_T(""), VK_LCONTROL);
		checkGetKeyName(_T(""), VK_RCONTROL);
		checkGetKeyName(_T(""), VK_LMENU);
		checkGetKeyName(_T(""), VK_RMENU);
		checkGetKeyName(_T(""), 0xB8);
		checkGetKeyName(_T(""), 0xB9);
		checkGetKeyName(_T(""), 0xC1);
		checkGetKeyName(_T(""), 0xC2);
	}
	
	TEST_METHOD(Letters) {
		for (BYTE c = 'A'; c <= 'Z'; c++) {
			checkGetKeyName(StringPrintf(_T("%c"), TCHAR(c)), c);
		}
	}
	
	TEST_METHOD(Digits) {
		for (BYTE c = '0'; c <= '9'; c++) {
			checkGetKeyName(StringPrintf(_T("%c"), TCHAR(c)), c);
		}
	}
	
	TEST_METHOD(OEM) {
		checkGetKeyName(_T(";"), VK_OEM_1);
		checkGetKeyName(_T("="), VK_OEM_PLUS);
		checkGetKeyName(_T(","), VK_OEM_COMMA);
		checkGetKeyName(_T("-"), VK_OEM_MINUS);
		checkGetKeyName(_T("."), VK_OEM_PERIOD);
		checkGetKeyName(_T("/"), VK_OEM_2);
		checkGetKeyName(_T("`"), VK_OEM_3);
		checkGetKeyName(_T("["), VK_OEM_4);
		checkGetKeyName(_T("\\"), VK_OEM_5);
		checkGetKeyName(_T("]"), VK_OEM_6);
		checkGetKeyName(_T("'"), VK_OEM_7);
		checkGetKeyName(_T("#223"), VK_OEM_8);
		checkGetKeyName(_T("#225"), VK_OEM_AX);
		checkGetKeyName(_T("#226"), VK_OEM_102);  // same name as VK_OEM_5 by default: "\"
		checkGetKeyName(_T("#240"), VK_OEM_ATTN);
	}
	
	TEST_METHOD(Media) {
		checkGetKeyName(_T("#166"), VK_BROWSER_BACK);
		checkGetKeyName(_T("#172"), VK_BROWSER_HOME);
		
		checkGetKeyName(_T("#173"), VK_VOLUME_MUTE);
		checkGetKeyName(_T("#183"), VK_LAUNCH_APP2);
	}
	
	TEST_METHOD(Japanese) {
		checkGetKeyName(_T("#21"), VK_KANA);
		checkGetKeyName(_T("#23"), VK_JUNJA);
		checkGetKeyName(_T("#24"), VK_FINAL);
		checkGetKeyName(_T("#25"), VK_HANJA);
	}
	
	TEST_METHOD(Other) {
		checkGetKeyName(_T("Backspace"), VK_BACK);
		checkGetKeyName(_T("Tab"), VK_TAB);
		checkGetKeyName(_T("Num 5"), VK_CLEAR);
		checkGetKeyName(_T("Enter"), VK_RETURN);
		
		checkGetKeyName(_T("#19"), VK_PAUSE);
		checkGetKeyName(_T("Caps Lock"), VK_CAPITAL);
		
		checkGetKeyName(_T("Esc"), VK_ESCAPE);
		
		checkGetKeyName(_T("Space"), VK_SPACE);
		checkGetKeyName(_T("Page Up"), VK_PRIOR);
		checkGetKeyName(_T("Left"), VK_LEFT);
		checkGetKeyName(_T("Insert"), VK_INSERT);
		checkGetKeyName(_T("Delete"), VK_DELETE);
		checkGetKeyName(_T("#47"), VK_HELP);
		
		checkGetKeyName(_T("Num 0"), VK_NUMPAD0);
		checkGetKeyName(_T("Num *"), VK_MULTIPLY);
		checkGetKeyName(_T("Num +"), VK_ADD);
		checkGetKeyName(_T("#108"), VK_SEPARATOR);
		checkGetKeyName(_T("Num -"), VK_SUBTRACT);
		checkGetKeyName(_T("Num Del"), VK_DECIMAL);
		checkGetKeyName(_T("Num /"), VK_DIVIDE);
		checkGetKeyName(_T("F1"), VK_F1);
		checkGetKeyName(_T("F12"), VK_F12);
		checkGetKeyName(_T("F13"), VK_F13);
		checkGetKeyName(_T("F24"), VK_F24);
		
		checkGetKeyName(_T("Num Lock"), VK_NUMLOCK);
		checkGetKeyName(_T("Scroll Lock"), VK_SCROLL);
	}
	
private:
	
	void checkGetKeyName(LPCTSTR expected, UINT vk) {
		Assert::AreEqual(
			expected, Keystroke::getKeyName(vk),
			StringPrintf(_T("getKeyName mismatch for: vk=0x%02X (%d)"), vk, vk));
	}
};

TEST_CLASS(CompareTest) {
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
		
		Assert::AreEqual(keystroke - keystrokes, static_cast<ptrdiff_t>(arrayLength(keystrokes)));
		
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
};

TEST_CLASS(SpecialKeysTest) {
public:
	
	TEST_METHOD(VkRight) {
		for (int i = 0; i < arrayLength(e_special_keys); i++) {
			const SpecialKey& special_key = e_special_keys[i];
			Assert::AreEqual(static_cast<BYTE>(special_key.vk_left + 1), special_key.vk_right);
		}
	}
};

}
