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
#include "../App.h"
#include "../Global.h"

namespace GlobalTest {

TEST_CLASS(MacrosTest) {
public:
	
	TEST_METHOD(ArrayLength_oneElement) {
		const int array[] = { 42 };
		Assert::AreEqual(size_t(1), arrayLength(array));
	}
	
	TEST_METHOD(ArrayLength_eightElements) {
		const int array[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
		Assert::AreEqual(size_t(8), arrayLength(array));
	}
	
	TEST_METHOD(ArrayLength_stringPointer) {
		const char* const string = "test";
		Assert::AreEqual(sizeof(char*), arrayLength(string));
	}
	
	TEST_METHOD(ArrayLength_charArrayString) {
		const char string[] = "test";
		Assert::AreEqual(size_t(5), arrayLength(string));
	}
	
	
	TEST_METHOD(ToBool_trueIsTrue) {
		Assert::IsTrue(toBool(true));
	}
	
	TEST_METHOD(ToBool_oneIsTrue) {
		Assert::IsTrue(toBool(1));
	}
	
	TEST_METHOD(ToBool_falseIsFalse) {
		Assert::IsFalse(toBool(false));
	}
	
	TEST_METHOD(ToBool_positiveNumberIsTrue) {
		Assert::IsTrue(toBool(42));
	}
	
	TEST_METHOD(ToBool_zeroIsFalse) {
		Assert::IsFalse(toBool(0));
	}
	
	TEST_METHOD(ToBool_negativeNumberIsTrue) {
		Assert::IsTrue(toBool(-42));
	}
	
	TEST_METHOD(ToBool_nullIsFalse) {
		Assert::IsFalse(toBool(NULL));
	}
	
	
	int executeVERIFP(bool condition) {
		VERIFP(condition, 1);
		return 2;
	}
	
	TEST_METHOD(VERIFP_true) {
		Assert::AreEqual(2, executeVERIFP(true));
	}
	
	TEST_METHOD(VERIFP_false) {
		Assert::AreEqual(1, executeVERIFP(false));
	}
	
	bool executeVERIFP_complexExpression() {
		VERIFP(1 + (2 * 3) == 7, false);
		return true;
	}
	
	TEST_METHOD(VERIFP_complexExpression) {
		Assert::IsTrue(executeVERIFP_complexExpression());
	}
	
	int checkVERIFP_countsEvaluation(int& value, int& result) {
		VERIFP(value++, result++);
		return 0;
	}
	
	TEST_METHOD(VERIFP_evaluatesConditionOnce) {
		int condition = 0;
		int result = 0;
		checkVERIFP_countsEvaluation(condition, result);
		Assert::AreEqual(1, condition);
	}
	
	TEST_METHOD(VERIFP_evaluatesResultOnce) {
		int condition = 0;
		int result = 0;
		checkVERIFP_countsEvaluation(condition, result);
		Assert::AreEqual(1, result);
	}
	
	
	bool executeVERIF(int& condition) {
		VERIF(condition++);
		return true;
	}
	
	TEST_METHOD(VERIF_true) {
		int condition = 1;
		Assert::IsTrue(executeVERIF(condition));
	}
	
	TEST_METHOD(VERIF_false) {
		int condition = 0;
		Assert::IsFalse(executeVERIF(condition));
	}
	
	TEST_METHOD(VERIF_evaluatesConditionOnce) {
		int condition = 0;
		executeVERIF(condition);
		Assert::AreEqual(1, condition);
	}
	
	
	void executeVERIFV(int& condition, bool& result) {
		result = false;
		VERIFV(condition++);
		result = true;
	}
	
	TEST_METHOD(VERIFV_true) {
		int condition = 1;
		bool result;
		executeVERIFV(condition, result);
		Assert::IsTrue(result);
	}
	
	TEST_METHOD(VERIFV_false) {
		int condition = 0;
		bool result;
		executeVERIFV(condition, result);
		Assert::IsFalse(result);
	}
	
	TEST_METHOD(VERIFV_evaluatesConditionOnce) {
		int condition = 0;
		bool result;
		executeVERIFV(condition, result);
		Assert::AreEqual(1, condition);
	}
	
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
};

TEST_CLASS(TokensTest) {
public:
	
	TEST_METHOD(AllTokensSet) {
		for (int lang = 0; lang < i18n::langCount; lang++) {
			i18n::setLanguage(lang);
			for (int tok = 0; tok < tokNotFound; tok++) {
				Assert::AreNotEqual(_T(""), getToken(tok));
			}
		}
	}
	
	TEST_METHOD(LanguageTokensAllDifferent) {
		for (int tok = 0; tok < tokNotFound; tok++) {
			Assert::AreNotEqual(_T(""), getToken(tok));
		}
	}
	
	TEST_METHOD(SomeFrenchTokensDifferentFromEnglish) {
		for (int tok = 0; tok < tokNotFound; tok++) {
			if (tok != tokShortcut && tok != tokAllProgramsBut) {
				continue;
			}
			i18n::setLanguage(i18n::langEN);
			LPCTSTR english_token = getToken(tok);
			i18n::setLanguage(i18n::langFR);
			LPCTSTR french_token = getToken(tok);
			Assert::AreNotEqual(english_token, french_token);
		}
	}
	
	TEST_METHOD(PolishTokensDefaultToEnglish) {
		for (int tok = 0; tok < tokNotFound; tok++) {
			if (tok == tokLanguageName) {
				continue;
			}
			i18n::setLanguage(i18n::langEN);
			LPCTSTR english_token = getToken(tok);
			i18n::setLanguage(i18n::langPL);
			LPCTSTR polish_token = getToken(tok);
			Assert::AreEqual(english_token, polish_token);
		}
	}
};

TEST_CLASS(ShellApiTest) {
public:
	
	TEST_METHOD(listUwpApps_succeeds) {
		int app_count;
		listUwpApps(
			[&](LPCTSTR name, LPCTSTR app_id, LPITEMIDLIST pidl) {
				app_count++;
				Assert::AreNotEqual(_T(""), name);
				Assert::AreNotEqual(_T(""), app_id);
				Assert::IsNotNull(pidl);
			});
		Assert::AreNotEqual(0, app_count);
	}
};

TEST_CLASS(GetSemiColonTokenTest) {
public:
	
	TEST_METHOD(FirstToken) {
		TCHAR tokens[] = _T("first;middle;last;");
		LPTSTR next_token = tokens;
		LPCTSTR current_token = getSemiColonToken(next_token);
		Assert::AreEqual(_T("first"), current_token);
		Assert::AreEqual(_T("middle;last;"), next_token);
		Assert::AreSame(tokens[0], *current_token);
		Assert::AreSame(tokens[6], *next_token);
	}
	
	TEST_METHOD(MiddleToken) {
		TCHAR tokens[] = _T("first\0middle;last;");
		LPTSTR next_token = tokens + 6;
		LPCTSTR current_token = getSemiColonToken(next_token);
		Assert::AreEqual(_T("middle"), current_token);
		Assert::AreEqual(_T("last;"), next_token);
		Assert::AreSame(tokens[6], *current_token);
		Assert::AreSame(tokens[13], *next_token);
	}
	
	TEST_METHOD(LastToken) {
		TCHAR tokens[] = _T("first\0middle\0last;");
		LPTSTR next_token = tokens + 13;
		LPCTSTR current_token = getSemiColonToken(next_token);
		Assert::AreEqual(_T("last"), current_token);
		Assert::AreEqual(_T(""), next_token);
		Assert::AreSame(tokens[13], *current_token);
		Assert::AreSame(tokens[18], *next_token);
	}
	
	TEST_METHOD(LastTokenWithoutSeparator) {
		TCHAR tokens[] = _T("first\0middle\0last");
		LPTSTR next_token = tokens + 13;
		LPCTSTR current_token = getSemiColonToken(next_token);
		Assert::AreEqual(_T("last"), current_token);
		Assert::AreEqual(_T(""), next_token);
		Assert::AreSame(tokens[13], *current_token);
		Assert::AreSame(tokens[17], *next_token);
	}
	
	TEST_METHOD(EmptyToken) {
		TCHAR tokens[] = _T("previous\0;next;");
		LPTSTR next_token = tokens + 9;
		LPCTSTR current_token = getSemiColonToken(next_token);
		Assert::AreEqual(_T(""), current_token);
		Assert::AreEqual(_T("previous"), tokens);
		Assert::AreEqual(_T(""), tokens + 9);
		Assert::AreEqual(_T("next;"), tokens + 10);
		Assert::AreSame(tokens[9], *current_token);
		Assert::AreSame(tokens[10], *next_token);
	}
};

TEST_CLASS(ClipboardTest) {
public:
	
	TEST_METHOD_INITIALIZE(setUp) {
		SetEnvironmentVariable(clipboard_env_variable, NULL);
	}
	
	TEST_METHOD(ClipboardToEnvironment_ansi) {
		setClipboardData(CF_TEXT, "A test string", 14);
		clipboardToEnvironment();
		checkClipboardEnvVariable(_T("A test string"));
	}
	
	TEST_METHOD(ClipboardToEnvironment_unicode) {
		setClipboardData(CF_UNICODETEXT, L"A test string", 28);
		clipboardToEnvironment();
		checkClipboardEnvVariable(_T("A test string"));
	}
	
	TEST_METHOD(ClipboardToEnvironment_emptyClipboard) {
		setClipboardData(CF_TEXT, NULL, 0);
		clipboardToEnvironment();
		checkClipboardEnvVariable(_T(""));
	}
	
private:
	
	void setClipboardData(UINT format, const void* data, size_t size) {
		Assert::IsTrue(OpenClipboard(NULL));
		Assert::IsTrue(EmptyClipboard());
		if (data) {
			const HGLOBAL clipboard_mem = GlobalAlloc(GMEM_MOVEABLE, size);
			memcpy(GlobalLock(clipboard_mem), data, size);
			GlobalUnlock(clipboard_mem);
			Assert::IsNotNull(SetClipboardData(format, clipboard_mem));
		}
		Assert::IsTrue(CloseClipboard());
	}
	
	void checkClipboardEnvVariable(LPCTSTR expected) {
		TCHAR actual[256];
		GetEnvironmentVariable(clipboard_env_variable, actual, arrayLength(actual));
		Assert::AreEqual(expected, actual);
	}
};

TEST_CLASS(MatchWildcardsTest) {
public:
	
	TEST_METHOD(Empty_matches_empty) {
		Assert::IsTrue(matchWildcards(_T(""), _T("")));
	}
	
	TEST_METHOD(Empty_mismatches_notEmpty) {
		Assert::IsFalse(matchWildcards(_T(""), _T("Subject")));
	}
	
	TEST_METHOD(SameString_matches) {
		Assert::IsTrue(matchWildcards(_T("Test"), _T("Test")));
	}
	
	TEST_METHOD(CaseMatters) {
		Assert::IsFalse(matchWildcards(_T("test"), _T("Test")));
		Assert::IsFalse(matchWildcards(_T("test"), _T("TEST")));
	}
	
	TEST_METHOD(QuestionMark_one) {
		Assert::IsTrue(matchWildcards(_T("te?t"), _T("test")));
	}
	
	TEST_METHOD(QuestionMark_two) {
		Assert::IsTrue(matchWildcards(_T("?e?t"), _T("test")));
	}
	
	TEST_METHOD(QuestionMark_needsOtherCharacterSame) {
		Assert::IsFalse(matchWildcards(_T("te?t"), _T("xxxx")));
	}
	
	TEST_METHOD(QuestionMark_needsSameLength) {
		Assert::IsTrue(matchWildcards(_T("????"), _T("test")));
		Assert::IsFalse(matchWildcards(_T("?????"), _T("test")));
		Assert::IsFalse(matchWildcards(_T("???"), _T("test")));
	}
	
	TEST_METHOD(Start_atTheBeginning) {
		Assert::IsTrue(matchWildcards(_T("*end"), _T("blah end")));
		Assert::IsFalse(matchWildcards(_T("*end"), _T("blah Xnd")));
	}
	
	TEST_METHOD(Star_atTheEnd) {
		Assert::IsTrue(matchWildcards(_T("beginning*"), _T("beginning blah")));
		Assert::IsFalse(matchWildcards(_T("beginning*"), _T("beginninX blah")));
	}
	
	TEST_METHOD(Star_beginningAndEnd) {
		Assert::IsTrue(matchWildcards(_T("*middle*"), _T("first middle last")));
		Assert::IsFalse(matchWildcards(_T("*middle*"), _T("first Xiddle last")));
		Assert::IsFalse(matchWildcards(_T("*middle*"), _T("first middlX last")));
	}
	
	TEST_METHOD(Star_middle) {
		Assert::IsTrue(matchWildcards(_T("first*last"), _T("first blah last")));
		Assert::IsFalse(matchWildcards(_T("first*last"), _T("firsX blah last")));
		Assert::IsFalse(matchWildcards(_T("first*last"), _T("first blah Xast")));
	}
	
	TEST_METHOD(PatternEnd_noWildcards) {
		const LPCTSTR pattern = _T("testXXX");
		Assert::IsTrue(matchWildcards(pattern, _T("test"), pattern + 4));
		Assert::IsFalse(matchWildcards(pattern, _T("test"), pattern + 3));
	}
	
	TEST_METHOD(PatternEnd_withStar) {
		const LPCTSTR pattern = _T("te*tXXX");
		Assert::IsTrue(matchWildcards(pattern, _T("tessst"), pattern + 4));
		Assert::IsFalse(matchWildcards(pattern, _T("tessst"), pattern + 5));
	}
	
	TEST_METHOD(PatternEnd_withStarAndQuestionMark) {
		const LPCTSTR pattern = _T("*tl?XXX");
		Assert::IsTrue(matchWildcards(pattern, _T("Title"), pattern + 4));
	}
};

TEST_CLASS(GlobalTest) {
public:
	
	TEST_METHOD(SkipUntilComma_noCommaNoUnescape) {
		TCHAR buffer[100];
		lstrcpy(buffer, _T("No \\\\ comma"));
		LPTSTR chr_ptr = buffer;
		skipUntilComma(chr_ptr, false);
		Assert::AreSame(buffer[11], *chr_ptr);
		Assert::AreEqual(_T("No \\\\ comma"), buffer);
	}
	
	TEST_METHOD(SkipUntilComma_commaNoUnescape) {
		TCHAR buffer[100];
		lstrcpy(buffer, _T("With \\\\ a, comma, end"));
		LPTSTR chr_ptr = buffer;
		skipUntilComma(chr_ptr, false);
		Assert::AreSame(buffer[11], *chr_ptr);
		Assert::AreEqual(_T("With \\\\ a"), buffer);
		Assert::AreEqual(_T("comma, end"), buffer + 11);
	}
	
	TEST_METHOD(SkipUntilComma_noCommaUnescape) {
		TCHAR buffer[100];
		lstrcpy(buffer, _T("No \\,\\\\ comma"));
		LPTSTR chr_ptr = buffer;
		skipUntilComma(chr_ptr, true);
		Assert::AreSame(buffer[13], *chr_ptr);
		Assert::AreEqual(_T("No ,\\ comma"), buffer);
	}
	
	TEST_METHOD(SkipUntilComma_commaUnescape) {
		TCHAR buffer[100];
		lstrcpy(buffer, _T("With \\,\\\\ a, comma, \\,\\\\ end"));
		LPTSTR chr_ptr = buffer;
		skipUntilComma(chr_ptr, true);
		Assert::AreSame(buffer[13], *chr_ptr);
		Assert::AreEqual(_T("With ,\\ a"), buffer);
		Assert::AreEqual(_T("comma, \\,\\\\ end"), buffer + 13);
	}
};

TEST_CLASS(WindowTest) {
public:
	
	TEST_METHOD_INITIALIZE(setUp) {
		m_hwnd = CreateWindow(_T("Edit"), NULL, 0, 0, 0, 0, 0, NULL, NULL, e_hInst, NULL);
	}
	
	TEST_METHOD_CLEANUP(tearDown) {
		if (m_hwnd) {
			DestroyWindow(m_hwnd);
		}
	}
	
	TEST_METHOD(CheckWindowClass_matchesEquals) {
		Assert::IsTrue(checkWindowClass(m_hwnd, _T("Edit"), false));
	}
	
	TEST_METHOD(CheckWindowClass_matchesSamePrefix) {
		Assert::IsTrue(checkWindowClass(m_hwnd, _T("Ed"), true));
	}
	
	TEST_METHOD(CheckWindowClass_samePrefixDoesNotMatchEquals) {
		Assert::IsFalse(checkWindowClass(m_hwnd, _T("Ed"), false));
	}
	
	TEST_METHOD(CheckWindowClass_doesNotMatchEquals) {
		Assert::IsFalse(checkWindowClass(m_hwnd, _T("Other"), false));
	}
	
	TEST_METHOD(CheckWindowClass_doesNotMatchSamePrefix) {
		Assert::IsFalse(checkWindowClass(m_hwnd, _T("EdX"), true));
	}
	
	TEST_METHOD(CheckWindowClass_windowDoesNotExist) {
		DestroyWindow(m_hwnd);
		const HWND hwnd = m_hwnd;
		m_hwnd = NULL;
		Assert::IsFalse(checkWindowClass(hwnd, _T("EDIT"), false));
	}
	
private:
	
	HWND m_hwnd;
	
};

TEST_CLASS(GlobalWindowTest) {
public:
	
	TEST_METHOD_INITIALIZE(setUp) {
		m_hdlgSendKeys = CreateDialog(e_hInst, MAKEINTRESOURCE(IDD_SENDKEYS), NULL, dialogProc);
	}
	
	TEST_METHOD_CLEANUP(tearDown) {
		DestroyWindow(m_hdlgSendKeys);
	}
	
	TEST_METHOD(GetDlgItemText) {
		SetDlgItemText(m_hdlgSendKeys, IDCTXT, _T("Test"));
		String str;
		getDlgItemText(m_hdlgSendKeys, IDCTXT, str);
		Assert::AreEqual(_T("Test"), str);
	}
	
	TEST_METHOD(GetWindowProcessName) {
		TCHAR actual_process_name[MAX_PATH];
		Assert::IsTrue(getWindowProcessName(m_hdlgSendKeys, actual_process_name));
		
		TCHAR current_process_name[MAX_PATH];
		Assert::AreNotEqual(DWORD(0), GetModuleFileName(NULL, current_process_name, MAX_PATH));
		PathStripPath(current_process_name);
		
		Assert::AreEqual(current_process_name, actual_process_name);
	}
	
private:
	
	HWND m_hdlgSendKeys;
	
	static INT_PTR CALLBACK dialogProc(
			HWND UNUSED(hdlg), UINT UNUSED(message), WPARAM UNUSED(wParam), LPARAM UNUSED(lParam)) {
		return FALSE;
	}
};

}
