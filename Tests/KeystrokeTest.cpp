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
namespace {

constexpr bool kUnsided = false;
constexpr bool kSided = true;

void useEnglishKeyNames() {
	i18n::setLanguage(i18n::kLangEN);
}

Keystroke::Condition parseCondition(LPCTSTR condition, TCHAR expected_name) {
	Assert::AreEqual(expected_name, condition[0], _T("Malformed condition"));
	switch (condition[1]) {
		case '/':
			return Keystroke::Condition::kIgnore;
		case '+':
			return Keystroke::Condition::kYes;
		case '-':
			return Keystroke::Condition::kNo;
		default:
			Assert::Fail(StringPrintf(_T("Invalid condition character: %c"), condition[1]));
	}
}

Keystroke buildKeystroke(
		BYTE vk, bool sided = kUnsided, DWORD sided_mod_code = 0,
		LPCTSTR conditions = _T("C/N/S/")) {
	Keystroke keystroke;
	keystroke.m_vk = vk;
	keystroke.m_sided = sided;
	keystroke.m_sided_mod_code = sided_mod_code;
	Assert::AreEqual(6, lstrlen(conditions));
	keystroke.m_conditions[Keystroke::kCondTypeCapsLock] = parseCondition(conditions, 'C');
	keystroke.m_conditions[Keystroke::kCondTypeNumLock] = parseCondition(conditions + 2, 'N');
	keystroke.m_conditions[Keystroke::kCondTypeScrollLock] = parseCondition(conditions + 4, 'S');
	return keystroke;
}

}  // namespace


TEST_CLASS(SpecialKeysTest) {
public:
	
	TEST_METHOD(VkRight) {
		for (const auto& special_key : kSpecialKeys) {
			Assert::AreEqual(BYTE(special_key.vk_left + 1), special_key.vk_right);
		}
	}
};


// GetDisplayName and ParseDisplayName
TEST_CLASS(DisplayNameTest) {
public:
	
	TEST_CLASS_INITIALIZE(classSetUp) {
		useEnglishKeyNames();
	}
	
	TEST_METHOD(Empty) {
		checkParseDisplayName(_T(""), buildKeystroke(0));
	}
	
	TEST_METHOD(Letter) {
		checkDisplayName(_T("X"), buildKeystroke('X'));
	}
	
	TEST_METHOD(NonCharacter) {
		checkDisplayName(_T("Page Up"), buildKeystroke(VK_PRIOR));
	}
	
	TEST_METHOD(NumericCode) {
		checkDisplayName(_T("#166"), buildKeystroke(VK_BROWSER_BACK));
	}
	
	TEST_METHOD(SpecialKeys_unsided) {
		checkDisplayName(_T("Win + X"), buildKeystroke('X', kUnsided, MOD_WIN));
		checkDisplayName(_T("Ctrl + X"), buildKeystroke('X', kUnsided, MOD_CONTROL));
		checkDisplayName(_T("Shift + X"), buildKeystroke('X', kUnsided, MOD_SHIFT));
		checkDisplayName(_T("Alt + X"), buildKeystroke('X', kUnsided, MOD_ALT));
		checkDisplayName(
			_T("Win + Ctrl + Shift + Alt + X"),
			buildKeystroke('X', kUnsided, MOD_WIN | MOD_CONTROL | MOD_SHIFT | MOD_ALT));
	}
	
	TEST_METHOD(SpecialKeys_sided) {
		checkDisplayName(_T("X"), buildKeystroke('X', kSided), /* check_parse= */ false);
		checkDisplayName(_T("Win Left + X"), buildKeystroke('X', kSided, MOD_WIN));
		checkDisplayName(_T("Ctrl Right + X"), buildKeystroke('X', kSided, MOD_CONTROL << kRightModCodeOffset));
		checkDisplayName(_T("Shift Left + X"), buildKeystroke('X', kSided, MOD_SHIFT));
		checkDisplayName(_T("Alt Right + X"), buildKeystroke('X', kSided, MOD_ALT << kRightModCodeOffset));
		checkDisplayName(
			_T("Win Right + Ctrl Left + Shift Right + Alt Left + X"),
			buildKeystroke('X', kSided, MOD_CONTROL | MOD_ALT | ((MOD_WIN | MOD_SHIFT) << kRightModCodeOffset)));
		checkDisplayName(
			_T("Win Right + Ctrl Left + Ctrl Right + Shift Right + Alt Left + Alt Right + X"),
			buildKeystroke('X', kSided, MOD_CONTROL | MOD_ALT | ((MOD_WIN | MOD_CONTROL | MOD_SHIFT | MOD_ALT) << kRightModCodeOffset)));
	}
	
	TEST_METHOD(SpecialKeys_only) {
		checkDisplayName(_T("Ctrl + "), buildKeystroke(0, kUnsided, MOD_CONTROL));
		checkDisplayName(
			_T("Win Right + Ctrl Left + Ctrl Right + Shift Right + Alt Left + Alt Right + "),
			buildKeystroke(0, kSided, MOD_CONTROL | MOD_ALT | ((MOD_WIN | MOD_CONTROL | MOD_SHIFT | MOD_ALT) << kRightModCodeOffset)));
	}
	
	TEST_METHOD(GetIgnoresConditions) {
		checkDisplayName(_T("Win + X"), buildKeystroke('X', kUnsided, MOD_WIN, _T("C+N-S+")), /* check_parse= */ false);
	}
	
private:
	
	void checkDisplayName(LPCTSTR display_name, const Keystroke& keystroke, bool check_parse = true) {
		// Verify getDisplayName().
		TCHAR generated_display_name[kHotKeyBufSize];
		keystroke.getDisplayName(generated_display_name);
		Assert::AreEqual(
			display_name, generated_display_name,
			StringPrintf(_T("getDisplayName mismatch for %s"), keystroke.rawDebugString().getSafe()));
		
		// Verify parseDisplayName().
		if (check_parse) {
			checkParseDisplayName(display_name, keystroke);
		}
	}
	
	void checkParseDisplayName(LPCTSTR display_name, const Keystroke& expected) {
		Keystroke parsed_keystroke;
		parsed_keystroke.parseDisplayName(String(display_name).getBuffer(1));
		
		Assert::AreEqual(
			expected.rawDebugString().getSafe(),
			parsed_keystroke.rawDebugString().getSafe(),
			StringPrintf(_T("parseDisplayName mismatch for: %s"), display_name));
	}
};


TEST_CLASS(ParseDisplayNameTest) {
public:
	
	TEST_CLASS_INITIALIZE(classSetUp) {
		useEnglishKeyNames();
	}
	
	TEST_METHOD(Whitespace_only) {
		checkParseDisplayName(_T(" "), buildKeystroke(0));
		checkParseDisplayName(_T("    "), buildKeystroke(0));
	}
	
	TEST_METHOD(KeySeparators_ignoresWhitespace) {
		checkParseDisplayName(_T("Win+Ctrl+X"),
			buildKeystroke('X', kUnsided, MOD_WIN | MOD_CONTROL));
		checkParseDisplayName(_T("Win  Right  +   Ctrl  Left  +   X"),
			buildKeystroke('X', kSided, (MOD_WIN << kRightModCodeOffset) | MOD_CONTROL));
	}
	
	TEST_METHOD(KeySeparators_plusOptional) {
		checkParseDisplayName(_T("Win X"),
			buildKeystroke('X', kUnsided, MOD_WIN));
		checkParseDisplayName(_T("Win Right Ctrl Left X"),
			buildKeystroke('X', kSided, (MOD_WIN << kRightModCodeOffset) | MOD_CONTROL));
	}
	
	TEST_METHOD(KeySeparators_atMostOnePlus) {
		checkParseDisplayName(_T("Win ++ X"), buildKeystroke(0, kUnsided, MOD_WIN));
		checkParseDisplayName(_T("Win + + X"), buildKeystroke(0, kUnsided, MOD_WIN));
	}
	
	TEST_METHOD(SpecialKeys_only) {
		checkParseDisplayName(_T("Ctrl"), buildKeystroke(0, kUnsided, MOD_CONTROL));
		checkParseDisplayName(
			_T("Win Right + Ctrl Left + Shift Right + Alt Left"),
			buildKeystroke(0, kSided, MOD_CONTROL | MOD_ALT | ((MOD_WIN | MOD_SHIFT) << kRightModCodeOffset)));
	}
	
	TEST_METHOD(SpecialKeys_sidedAndUnsided) {
		checkParseDisplayName(_T("Ctrl + Shift Right + X"),
			buildKeystroke('X', kSided, MOD_CONTROL | (MOD_SHIFT << kRightModCodeOffset)));
	}
	
	TEST_METHOD(SpecialKeys_sidedMixedOrder) {
		checkParseDisplayName(_T("Ctrl Right + Shift Left + Ctrl Left + X"),
			buildKeystroke('X', kSided, MOD_CONTROL | MOD_SHIFT | (MOD_CONTROL << kRightModCodeOffset)));
		checkParseDisplayName(_T("Ctrl Right + Shift + Ctrl + X"),
			buildKeystroke('X', kSided, MOD_CONTROL << kRightModCodeOffset));
	}
	
private:
	
	void checkParseDisplayName(LPCTSTR input, const Keystroke& expected) {
		Keystroke actual;
		actual.parseDisplayName(String(input).getBuffer(1));
		Assert::AreEqual(
			expected.rawDebugString().getSafe(), actual.rawDebugString().getSafe(),
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
		for (char c = 'A'; c <= 'Z'; c++) {
			checkGetKeyName(StringPrintf(_T("%c"), c), c);
		}
	}
	
	TEST_METHOD(Digits) {
		for (char c = '0'; c <= '9'; c++) {
			checkGetKeyName(StringPrintf(_T("%c"), c), c);
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


TEST_CLASS(IsSubsetTest) {
public:
	
	TEST_METHOD(Same) {
		const Keystroke keystrokes[] = {
			buildKeystroke('A', kUnsided, 0),
			buildKeystroke('A', kSided, 0),
			buildKeystroke('A', kUnsided, MOD_WIN),
			buildKeystroke('A', kSided, MOD_WIN),
			buildKeystroke('A', kUnsided, MOD_WIN, _T("C/N+S-")),
			buildKeystroke('A', kSided, MOD_WIN | (MOD_SHIFT << kRightModCodeOffset)),
			buildKeystroke('A', kSided, MOD_WIN | MOD_SHIFT | (MOD_SHIFT << kRightModCodeOffset)),
		};
		for (const auto& keystroke : keystrokes) {
			checkIsSubset(true, keystroke, keystroke);
		}
	}
	
	TEST_METHOD(Incompatible) {
		const std::vector<std::vector<Keystroke>> silos = {
			{
				buildKeystroke('A', kUnsided, 0),
				buildKeystroke('B', kUnsided, 0),
				buildKeystroke('A', kUnsided, MOD_WIN),
				buildKeystroke('A', kUnsided, MOD_SHIFT),
			}, {
				buildKeystroke('A', kUnsided, 0, _T("C+N/S/")),
				buildKeystroke('A', kUnsided, 0, _T("C-N/S/")),
				buildKeystroke('A', kUnsided, 0, _T("C/N+S/")),
				buildKeystroke('A', kUnsided, 0, _T("C/N/S-")),
			}, {
				buildKeystroke('A', kSided, MOD_SHIFT),
				buildKeystroke('A', kSided, MOD_SHIFT << kRightModCodeOffset),
				buildKeystroke('A', kSided, MOD_SHIFT | (MOD_SHIFT << kRightModCodeOffset)),
			},
		};
		for (const auto& silo : silos) {
			for (int i = 0; i < silo.size(); i++) {
				for (int j = 0; j < silo.size(); j++) {
					checkIsSubset(i == j, silo[i], silo[j]);
				}
			}
		}
	}
	
	TEST_METHOD(Nested) {
		const std::vector<std::vector<Keystroke>> nested_chains = {
			{
				buildKeystroke('A', kUnsided, MOD_SHIFT),
				buildKeystroke('A', kSided, MOD_SHIFT),
			}, {
				buildKeystroke('A', kUnsided, MOD_SHIFT),
				buildKeystroke('A', kSided, MOD_SHIFT << kRightModCodeOffset),
			}, {
				buildKeystroke('A', kUnsided, MOD_SHIFT),
				buildKeystroke('A', kSided, MOD_SHIFT | (MOD_SHIFT << kRightModCodeOffset)),
			}, {
				buildKeystroke('A', kUnsided, 0, _T("C/N/S/")),
				buildKeystroke('A', kUnsided, 0, _T("C-N/S/")),
				buildKeystroke('A', kUnsided, 0, _T("C-N+S/")),
				buildKeystroke('A', kUnsided, 0, _T("C-N+S-")),
			}, {
				buildKeystroke('A', kUnsided, MOD_SHIFT),
				buildKeystroke('A', kSided, MOD_SHIFT << kRightModCodeOffset),
				buildKeystroke('A', kSided, MOD_SHIFT << kRightModCodeOffset, _T("C-N/S/")),
			},
		};
		for (const auto& chain : nested_chains) {
			for (int i = 0; i < chain.size(); i++) {
				for (int j = 0; j < chain.size(); j++) {
					checkIsSubset(i <= j, chain[i], chain[j]);
				}
			}
		}
	}
	
private:
	
	void checkIsSubset(bool expected_result, const Keystroke& keystroke, const Keystroke& other) {
		Assert::AreEqual(
			expected_result, keystroke.isSubset(other),
			StringPrintf(_T("<%s> isSubset <%s>"), keystroke.debugString().getSafe(), other.debugString().getSafe()));
	}
};


TEST_CLASS(CompareTest) {
public:
	
	TEST_METHOD(Equal) {
		checkCompareOneWay(
			0,
			buildKeystroke('A', kUnsided, 0),
			buildKeystroke('A', kSided, 0));
	}
	
	TEST_METHOD(NoModCode) {
		checkCompareLess(
			buildKeystroke('A', kUnsided, 0),
			buildKeystroke('B', kUnsided, 0));
	}
	
	TEST_METHOD(SameModCode) {
		checkCompareLess(
			buildKeystroke('A', kUnsided, MOD_CONTROL | MOD_SHIFT),
			buildKeystroke('B', kUnsided, MOD_CONTROL | MOD_SHIFT));
	}
	
	TEST_METHOD(WinBeforeControl) {
		checkCompareLess(
			buildKeystroke('A', kUnsided, MOD_WIN),
			buildKeystroke('A', kUnsided, MOD_CONTROL));
	}
	
	TEST_METHOD(ControlBeforeShift) {
		checkCompareLess(
			buildKeystroke('A', kUnsided, MOD_CONTROL),
			buildKeystroke('A', kUnsided, MOD_SHIFT));
	}
	
	TEST_METHOD(ShiftBeforeAlt) {
		checkCompareLess(
			buildKeystroke('A', kUnsided, MOD_SHIFT),
			buildKeystroke('A', kUnsided, MOD_ALT));
	}
	
	TEST_METHOD(WinControlBeforeWinControlShift) {
		checkCompareLess(
			buildKeystroke('A', kUnsided, MOD_WIN | MOD_CONTROL),
			buildKeystroke('A', kUnsided, MOD_WIN | MOD_CONTROL | MOD_SHIFT));
	}
	
	TEST_METHOD(WinControlShiftBeforeWinControlAlt) {
		checkCompareLess(
			buildKeystroke('A', kUnsided, MOD_WIN | MOD_CONTROL | MOD_SHIFT),
			buildKeystroke('A', kUnsided, MOD_WIN | MOD_CONTROL | MOD_ALT));
	}
	
	TEST_METHOD(NoneBeforeUnsided) {
		checkCompareLess(
			buildKeystroke('A', kUnsided, 0),
			buildKeystroke('A', kUnsided, MOD_ALT));
	}
	
	TEST_METHOD(UnsidedBeforeLeft) {
		checkCompareLess(
			buildKeystroke('A', kUnsided, MOD_ALT),
			buildKeystroke('A', kSided, MOD_ALT));
	}
	
	TEST_METHOD(LeftBeforeRight) {
		checkCompareLess(
			buildKeystroke('A', kSided, MOD_ALT),
			buildKeystroke('A', kSided, MOD_ALT << kRightModCodeOffset));
	}
	
	TEST_METHOD(SortedList) {
		const Keystroke keystrokes[] = {
			buildKeystroke('W', kUnsided, 0),
			buildKeystroke('W', kUnsided, MOD_WIN),
			
			buildKeystroke('X', kUnsided, 0),
			
			buildKeystroke('X', kUnsided, MOD_WIN),
			buildKeystroke('X', kUnsided, MOD_WIN | MOD_CONTROL),
			buildKeystroke('X', kUnsided, MOD_WIN | MOD_CONTROL | MOD_SHIFT),
			buildKeystroke('X', kUnsided, MOD_WIN | MOD_SHIFT),
			buildKeystroke('X', kSided, MOD_WIN | MOD_SHIFT),
			buildKeystroke('X', kSided, MOD_WIN | (MOD_SHIFT << kRightModCodeOffset)),
			buildKeystroke('X', kSided, MOD_WIN | MOD_SHIFT | (MOD_SHIFT << kRightModCodeOffset)),
			
			buildKeystroke('X', kUnsided, MOD_CONTROL),
			buildKeystroke('X', kUnsided, MOD_CONTROL | MOD_SHIFT),
			buildKeystroke('X', kUnsided, MOD_CONTROL | MOD_SHIFT | MOD_ALT),
			buildKeystroke('X', kUnsided, MOD_CONTROL | MOD_ALT),
			buildKeystroke('X', kSided, MOD_CONTROL),
			
			buildKeystroke('X', kUnsided, MOD_SHIFT),
			buildKeystroke('X', kSided, MOD_SHIFT),
			buildKeystroke('X', kSided, MOD_SHIFT << kRightModCodeOffset),
			buildKeystroke('X', kSided, MOD_SHIFT | (MOD_SHIFT << kRightModCodeOffset)),
			
			buildKeystroke('X', kUnsided, MOD_ALT),
			buildKeystroke('X', kSided, MOD_ALT),
			buildKeystroke('X', kSided, MOD_ALT << kRightModCodeOffset),
			buildKeystroke('X', kSided, MOD_ALT | (MOD_ALT << kRightModCodeOffset)),
			
			buildKeystroke('Y', kUnsided, 0),
		};
		
		for (int i = 0; i < arrayLength(keystrokes); i++) {
			for (int j = 0; j < arrayLength(keystrokes); j++) {
				checkCompareOneWay(i - j, keystrokes[i], keystrokes[j]);
			}
		}
	}
	
private:
	
	void checkCompareLess(const Keystroke& keystroke1, const Keystroke& keystroke2) {
		checkCompareOneWay(-1, keystroke1, keystroke2);
		checkCompareOneWay(+1, keystroke2, keystroke1);
	}
	
	void checkCompareOneWay(int expected_result, const Keystroke& keystroke1, const Keystroke& keystroke2) {
		Assert::AreEqual(
			testing::normalizeCompareResult(expected_result),
			testing::normalizeCompareResult(Keystroke::compare(keystroke1, keystroke2)),
			StringPrintf(_T("Comparing <%s> and <%s>"),
				keystroke1.debugString().getSafe(), keystroke2.debugString().getSafe()));
	}
};

}  // namespace KeystrokeTest
