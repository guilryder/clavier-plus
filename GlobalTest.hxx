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

#include "App.h"
#include "Global.h"

class MacrosTest : public CxxTest::TestSuite {
private:
	
	class HasLess {
	public:
		HasLess(int value, int* construction_counter = NULL) : m_value(value) {
			if (construction_counter) {
				(*construction_counter)++;
			}
		}
		
		int getValue() const {
			return m_value;
		}
		
		bool operator < (const HasLess& other) const {
			return m_value < other.m_value;
		}
		
	private:
		int m_value;
	};
	
public:
	
	void testArrayLength_oneElement() {
		const int array[] = { 42 };
		TS_ASSERT_EQUALS(1, arrayLength(array));
	}
	
	void testArrayLength_eightElements() {
		const int array[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
		TS_ASSERT_EQUALS(8, arrayLength(array));
	}
	
	void testArrayLength_stringPointer() {
		const char* const string = "test";
		TS_ASSERT_EQUALS(sizeof(char*), arrayLength(string));
	}
	
	void testArrayLength_charArrayString() {
		const char string[] = "test";
		TS_ASSERT_EQUALS(5, arrayLength(string));
	}
	
	
	void testToBool_trueIsTrue() {
		TS_ASSERT_EQUALS(true, toBool(true));
	}
	
	void testToBool_oneIsTrue() {
		TS_ASSERT_EQUALS(true, toBool(1));
	}
	
	void testToBool_falseIsFalse() {
		TS_ASSERT_EQUALS(false, toBool(false));
	}
	
	void testToBool_positiveNumberIsTrue() {
		TS_ASSERT_EQUALS(true, toBool(42));
	}
	
	void testToBool_zeroIsFalse() {
		TS_ASSERT_EQUALS(false, toBool(0));
	}
	
	void testToBool_negativeNumberIsTrue() {
		TS_ASSERT_EQUALS(true, toBool(-42));
	}
	
	void testToBool_nullIsFalse() {
		TS_ASSERT_EQUALS(false, toBool(NULL));
	}
	
	
	void testMax_integers() {
		TS_ASSERT_EQUALS(5, max(3, 5));
	}
	
	void testMax_unsafeIntegers() {
		int a = 4;
		int b = 4;
		TS_ASSERT_DIFFERS(4, max(a++, (--b)--));
	}
	
	void testMax_usesLessOperator() {
		TS_ASSERT_EQUALS(5, max(HasLess(3), HasLess(5)).getValue());
	}
	
	void testMax_returnsCopy() {
		int construction_counter = 0;
		max(HasLess(42, &construction_counter), HasLess(3, &construction_counter));
		TS_ASSERT_LESS_THAN(2, construction_counter);
	}
	
	
	int executeVERIFP(bool condition) {
		VERIFP(condition, 1);
		return 2;
	}
	
	void testVERIFP_true() {
		TS_ASSERT_EQUALS(2, executeVERIFP(true));
	}
	
	void testVERIFP_false() {
		TS_ASSERT_EQUALS(1, executeVERIFP(false));
	}
	
	bool executeVERIFP_complexExpression() {
		VERIFP(1 + (2 * 3) == 7, false);
		return true;
	}
	
	void testVERIFP_complexExpression() {
		TS_ASSERT_EQUALS(true, executeVERIFP_complexExpression());
	}
	
	int checkVERIFP_countsEvaluation(int& value, int& result) {
		VERIFP(value++, result++);
		return 0;
	}
	
	void testVERIFP_evaluatesConditionOnce() {
		int condition = 0;
		int result = 0;
		checkVERIFP_countsEvaluation(condition, result);
		TS_ASSERT_EQUALS(1, condition);
	}
	
	void testVERIFP_evaluatesResultOnce() {
		int condition = 0;
		int result = 0;
		checkVERIFP_countsEvaluation(condition, result);
		TS_ASSERT_EQUALS(1, result);
	}
	
	
	bool executeVERIF(int& condition) {
		VERIF(condition++);
		return true;
	}
	
	void testVERIF_true() {
		int condition = 1;
		TS_ASSERT_EQUALS(true, executeVERIF(condition));
	}
	
	void testVERIF_false() {
		int condition = 0;
		TS_ASSERT_EQUALS(false, executeVERIF(condition));
	}
	
	void testVERIF_evaluatesConditionOnce() {
		int condition = 0;
		executeVERIF(condition);
		TS_ASSERT_EQUALS(1, condition);
	}
	
	
	void executeVERIFV(int& condition, bool& result) {
		result = false;
		VERIFV(condition++);
		result = true;
	}
	
	void testVERIFV_true() {
		int condition = 1;
		bool result;
		executeVERIFV(condition, result);
		TS_ASSERT_EQUALS(true, result);
	}
	
	void testVERIFV_false() {
		int condition = 0;
		bool result;
		executeVERIFV(condition, result);
		TS_ASSERT_EQUALS(false, result);
	}
	
	void testVERIFV_evaluatesConditionOnce() {
		int condition = 0;
		bool result;
		executeVERIFV(condition, result);
		TS_ASSERT_EQUALS(1, condition);
	}
};


class TokensTest : public CxxTest::TestSuite {
public:
	
	void testAllTokensSet() {
		for (int lang = 0; lang < i18n::langCount; lang++) {
			i18n::setLanguage(lang);
			for (int tok = 0; tok < tokNotFound; tok++) {
				TS_ASSERT(*getToken(tok));
			}
		}
	}
	
	void testLanguageTokensAllDifferent() {
		for (int tok = 0; tok < tokNotFound; tok++) {
			TS_ASSERT(*getToken(tok));
		}
	}
	
	void testSomeFrenchTokensDifferentFromEnglish() {
		for (int tok = 0; tok < tokNotFound; tok++) {
			if (tok != tokShortcut && tok != tokAllProgramsBut) {
				continue;
			}
			i18n::setLanguage(i18n::langEN);
			LPCTSTR english_token = getToken(tok);
			i18n::setLanguage(i18n::langFR);
			LPCTSTR french_token = getToken(tok);
			TS_ASSERT_DIFFERS(0, lstrcmpi(english_token, french_token));
		}
	}
	
	void testPolishTokensDefaultToEnglish() {
		for (int tok = 0; tok < tokNotFound; tok++) {
			if (tok == tokLanguageName) {
				continue;
			}
			i18n::setLanguage(i18n::langEN);
			LPCTSTR english_token = getToken(tok);
			i18n::setLanguage(i18n::langPL);
			LPCTSTR polish_token = getToken(tok);
			TS_ASSERT_EQUALS(0, lstrcmpi(english_token, polish_token));
		}
	}
};


class GetSemiColonTokenTest : public CxxTest::TestSuite {
public:
	
	void testFirstToken() {
		TCHAR tokens[] = _T("current;next;");
		LPTSTR next_token = tokens;
		LPCTSTR current_token = getSemiColonToken(next_token);
		TS_ASSERT_EQUALS(0, lstrcmp(current_token, _T("current")));
		TS_ASSERT_SAME_DATA(tokens, _T("current\0next"), 14);
		TS_ASSERT_EQUALS(current_token, tokens);
		TS_ASSERT_EQUALS(next_token, tokens + 8);
	}
	
	void testMiddleToken() {
		TCHAR tokens[] = _T("previous\0current;next;");
		LPTSTR next_token = tokens + 9;
		LPCTSTR current_token = getSemiColonToken(next_token);
		TS_ASSERT_EQUALS(0, lstrcmp(current_token, _T("current")));
		TS_ASSERT_SAME_DATA(tokens, _T("previous\0current\0next"), 23);
		TS_ASSERT_EQUALS(current_token, tokens + 9);
		TS_ASSERT_EQUALS(next_token, tokens + 17);
	}
	
	void testLastToken() {
		TCHAR tokens[] = _T("previous\0current;next;");
		LPTSTR next_token = tokens + 9;
		LPCTSTR current_token = getSemiColonToken(next_token);
		TS_ASSERT_EQUALS(0, lstrcmp(current_token, _T("current")));
		TS_ASSERT_SAME_DATA(tokens, _T("previous\0current\0next"), 23);
		TS_ASSERT_EQUALS(current_token, tokens + 9);
		TS_ASSERT_EQUALS(next_token, tokens + 17);
	}
	
	void testEmptyToken() {
		TCHAR tokens[] = _T("previous\0;next;");
		LPTSTR next_token = tokens + 9;
		LPCTSTR current_token = getSemiColonToken(next_token);
		TS_ASSERT_EQUALS(0, lstrcmp(current_token, _T("")));
		TS_ASSERT_SAME_DATA(tokens, _T("previous\0\0next;"), 16);
		TS_ASSERT_EQUALS(current_token, tokens + 9);
		TS_ASSERT_EQUALS(next_token, tokens + 10);
	}
};


class ClipboardTest : public CxxTest::TestSuite {
private:
	
	void setClipboardData(UINT format, const void* data, size_t size) {
		TS_ASSERT(OpenClipboard(NULL));
		TS_ASSERT(EmptyClipboard());
		if (data) {
			const HGLOBAL clipboard_mem = GlobalAlloc(GMEM_MOVEABLE, size);
			memcpy(GlobalLock(clipboard_mem), data, size);
			GlobalUnlock(clipboard_mem);
			TS_ASSERT_DIFFERS((HANDLE)NULL, SetClipboardData(format, clipboard_mem));
		}
		TS_ASSERT(CloseClipboard());
	}
	
	void checkClipboardEnvVariable(LPCTSTR expected) {
		TCHAR actual[256];
		GetEnvironmentVariable(clipboard_env_variable, actual, arrayLength(actual));
		TS_ASSERT_EQUALS(0, lstrcmp(expected, actual));
	}
	
public:
	
	void setUp() {
		SetEnvironmentVariable(clipboard_env_variable, NULL);
	}
	
	void testClipboardToEnvironment_ansi() {
		setClipboardData(CF_TEXT, "A test string", 14);
		clipboardToEnvironment();
		checkClipboardEnvVariable(_T("A test string"));
	}
	
	void testClipboardToEnvironment_unicode() {
		setClipboardData(CF_UNICODETEXT, L"A test string", 28);
		clipboardToEnvironment();
		checkClipboardEnvVariable(_T("A test string"));
	}
	
	void testClipboardToEnvironment_emptyClipboard() {
		setClipboardData(CF_TEXT, NULL, 0);
		clipboardToEnvironment();
		checkClipboardEnvVariable(_T(""));
	}
};


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


class GlobalTest : public CxxTest::TestSuite {
public:
	
	void testSkipUntilComma_noCommaNoUnescape() {
		TCHAR buffer[100];
		lstrcpy(buffer, _T("No \\\\ comma"));
		LPTSTR chr_ptr = buffer;
		skipUntilComma(chr_ptr, false);
		TS_ASSERT_EQUALS(buffer + 11, chr_ptr);
		TS_ASSERT_EQUALS(0, lstrcmp(_T("No \\\\ comma"), buffer));
	}
	
	void testSkipUntilComma_commaNoUnescape() {
		TCHAR buffer[100];
		lstrcpy(buffer, _T("With \\\\ a, comma, end"));
		LPTSTR chr_ptr = buffer;
		skipUntilComma(chr_ptr, false);
		TS_ASSERT_EQUALS(buffer + 11, chr_ptr);
		TS_ASSERT_EQUALS(0, lstrcmp(_T("With \\\\ a"), buffer));
		TS_ASSERT_EQUALS(0, lstrcmp(_T("comma, end"), buffer + 11));
	}
	
	void testSkipUntilComma_noCommaUnescape() {
		TCHAR buffer[100];
		lstrcpy(buffer, _T("No \\,\\\\ comma"));
		LPTSTR chr_ptr = buffer;
		skipUntilComma(chr_ptr, true);
		TS_ASSERT_EQUALS(buffer + 13, chr_ptr);
		TS_ASSERT_EQUALS(0, lstrcmp(_T("No ,\\ comma"), buffer));
	}
	
	void testSkipUntilComma_commaUnescape() {
		TCHAR buffer[100];
		lstrcpy(buffer, _T("With \\,\\\\ a, comma, \\,\\\\ end"));
		LPTSTR chr_ptr = buffer;
		skipUntilComma(chr_ptr, true);
		TS_ASSERT_EQUALS(buffer + 13, chr_ptr);
		TS_ASSERT_EQUALS(0, lstrcmp(_T("With ,\\ a"), buffer));
		TS_ASSERT_EQUALS(0, lstrcmp(_T("comma, \\,\\\\ end"), buffer + 13));
	}
};


class WindowTest : public CxxTest::TestSuite {
private:
	
	HWND m_hwnd;
	
public:
	
	void setUp() {
		m_hwnd = CreateWindow(_T("Edit"), NULL, 0, 0, 0, 0, 0, NULL, NULL, e_hInst, NULL);
	}
	
	void tearDown() {
		if (m_hwnd) {
			DestroyWindow(m_hwnd);
		}
	}
	
	void testCheckWindowClass_matchesEquals() {
		TS_ASSERT_EQUALS(true, checkWindowClass(m_hwnd, _T("Edit"), false));
	}
	
	void testCheckWindowClass_matchesSamePrefix() {
		TS_ASSERT_EQUALS(true, checkWindowClass(m_hwnd, _T("Ed"), true));
	}
	
	void testCheckWindowClass_samePrefixDoesNotMatchEquals() {
		TS_ASSERT_EQUALS(false, checkWindowClass(m_hwnd, _T("Ed"), false));
	}
	
	void testCheckWindowClass_doesNotMatchEquals() {
		TS_ASSERT_EQUALS(false, checkWindowClass(m_hwnd, _T("Other"), false));
	}
	
	void testCheckWindowClass_doesNotMatchSamePrefix() {
		TS_ASSERT_EQUALS(false, checkWindowClass(m_hwnd, _T("EdX"), true));
	}
	
	void testCheckWindowClass_windowDoesNotExist() {
		DestroyWindow(m_hwnd);
		const HWND hwnd = m_hwnd;
		m_hwnd = NULL;
		TS_ASSERT_EQUALS(false, checkWindowClass(hwnd, _T("EDIT"), false));
	}
};


class GlobalWindowTest : public CxxTest::TestSuite {
private:
	
	HWND m_hdlgSendKeys;
	
	static INT_PTR CALLBACK dialogProc(HWND MY_UNUSED(hdlg), UINT MY_UNUSED(message),
			WPARAM MY_UNUSED(wParam), LPARAM MY_UNUSED(lParam)) {
		return FALSE;
	}
	
public:
	
	void setUp() {
		m_hdlgSendKeys = CreateDialog(e_hInst, MAKEINTRESOURCE(IDD_SENDKEYS), NULL, dialogProc);
	}
	
	void tearDown() {
		DestroyWindow(m_hdlgSendKeys);
	}
	
	void testGetDlgItemText() {
		SetDlgItemText(m_hdlgSendKeys, IDCTXT, _T("Test"));
		String str;
		getDlgItemText(m_hdlgSendKeys, IDCTXT, str);
		TS_ASSERT_EQUALS(0, lstrcmp(_T("Test"), str));
	}
	
	void testGetWindowProcessName() {
		TCHAR actual_process_name[MAX_PATH];
		TS_ASSERT_EQUALS(true, getWindowProcessName(m_hdlgSendKeys, actual_process_name));
		TS_ASSERT_EQUALS(0, lstrcmp(_T("tests.exe"), actual_process_name));
	}
};
