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
#include "../i18n.h"
#include "../Shortcut.h"


namespace Microsoft::VisualStudio::CppUnitTestFramework {

template<> inline std::wstring ToString<i18n::Language>(const i18n::Language& lang) {
	RETURN_WIDE_STRING(lang);
}

template<> inline std::wstring ToString<Shortcut>(const Shortcut& sh) {
	return std::wstring(StringPrintf(
		_T("Shortcut{m_vk=0x%02x ('%c') m_programs=\"%s\" m_programs_only=%s}"),
		sh.m_vk, sh.m_vk,
		sh.m_programs.getSafe(),
		sh.m_programs_only ? _T("true") : _T("false")));
}

}  // Microsoft::VisualStudio::CppUnitTestFramework


namespace ShortcutTest {

TEST_CLASS(ShortcutGetProgramsTest) {
public:
	
	TEST_METHOD(Empty) {
		Assert::IsNull(m_shortcut.getPrograms());
	}
	
	TEST_METHOD(OnlySeparators) {
		m_shortcut.m_programs = _T(";");
		String* actual = m_shortcut.getPrograms();
		Assert::IsNotNull(actual);
		Assert::AreEqual(_T(""), actual[0]);
		delete [] actual;
	}
	
	TEST_METHOD(One) {
		m_shortcut.m_programs = _T("prog1");
		String* actual = m_shortcut.getPrograms();
		Assert::IsNotNull(actual);
		Assert::AreEqual(_T("prog1"), actual[0]);
		Assert::AreEqual(_T(""), actual[1]);
		delete [] actual;
	}
	
	TEST_METHOD(Several) {
		m_shortcut.m_programs = _T("prog1;prog2");
		String* actual = m_shortcut.getPrograms();
		Assert::IsNotNull(actual);
		Assert::AreEqual(_T("prog1"), actual[0]);
		Assert::AreEqual(_T("prog2"), actual[1]);
		Assert::AreEqual(_T(""), actual[2]);
		delete [] actual;
	}
	
	TEST_METHOD(SkipsEmpty) {
		m_shortcut.m_programs = _T(";prog1;;prog2;");
		String* actual = m_shortcut.getPrograms();
		Assert::IsNotNull(actual);
		Assert::AreEqual(_T("prog1"), actual[0]);
		Assert::AreEqual(_T("prog2"), actual[1]);
		Assert::AreEqual(_T(""), actual[2]);
		delete [] actual;
	}
	
private:
	
	Shortcut m_shortcut;
};

TEST_CLASS(ShortcutCleanProgramsTest) {
public:
	
	TEST_METHOD(Empty) {
		check(_T(""), _T(""));
	}
	
	TEST_METHOD(OnlySeparators) {
		check(_T(";"), _T(""));
		check(_T(";;"), _T(""));
	}
	
	TEST_METHOD(One) {
		check(_T("prog1"), _T("prog1"));
	}
	
	TEST_METHOD(Several) {
		check(_T("prog1;prog2"), _T("prog1;prog2"));
	}
	
	TEST_METHOD(SkipsEmpty) {
		check(_T(";prog1;;prog2;"), _T("prog1;prog2"));
	}
	
	TEST_METHOD(IgnoresCase) {
		check(_T("prog1;PrOg1;PROG1"), _T("prog1"));
	}
	
	TEST_METHOD(DropsDuplicates) {
		check(_T("prog1;prog2;prog1"), _T("prog1;prog2"));
		check(_T("prog1;prog1;prog1"), _T("prog1"));
		check(_T("prog1;prog2;prog1;prog3;prog1;prog2"), _T("prog1;prog2;prog3"));
		check(_T("prog1;prog2;prog3;prog2"), _T("prog1;prog2;prog3"));
	}
	
private:
	
	Shortcut m_shortcut;
	
	void check(LPCTSTR programsBefore, LPCTSTR programsExpectedAfter) {
		m_shortcut.m_programs = programsBefore;
		m_shortcut.cleanPrograms();
		Assert::AreEqual(programsExpectedAfter, m_shortcut.m_programs);
	}
};

TEST_CLASS(ShortcutMatchTest) {
public:
	
	TEST_METHOD_INITIALIZE(setUp) {
		m_ks.m_vk = 'A';
		m_ks.m_sided_mod_code = MOD_CONTROL;
	}
	
	TEST_METHOD(Basic) {
		Shortcut shortcut(m_ks);
		
		Keystroke ks_matching;
		ks_matching.m_vk = 'A';
		ks_matching.m_sided_mod_code = MOD_CONTROL;
		ks_matching.m_sided = true;
		
		Keystroke ks_different(m_ks);
		ks_different.m_vk = 'B';
		
		Assert::IsTrue(shortcut.isSubset(m_ks, /* other_program= */ nullptr));
		Assert::IsTrue(shortcut.isSubset(ks_matching, /* other_program= */ nullptr));
		Assert::IsTrue(shortcut.isSubset(m_ks, _T("prog")));
		Assert::IsFalse(shortcut.isSubset(ks_different, /* other_program= */ nullptr));
	}
	
	TEST_METHOD(ProgramsOnlyTrue) {
		testProgramsOnly(true);
	}
	
	TEST_METHOD(ProgramsOnlyFalse) {
		testProgramsOnly(false);
	}
	
	void testProgramsOnly(bool programsOnly) {
		Shortcut shortcut(m_ks);
		shortcut.m_programs_only = programsOnly;
		shortcut.m_programs = _T("prog1;prog2;prog3");
		
		Assert::AreEqual(!programsOnly, shortcut.isSubset(m_ks, /* other_program= */ nullptr));
		Assert::AreEqual(programsOnly, shortcut.isSubset(m_ks, _T("prog1")));
		Assert::AreEqual(programsOnly, shortcut.isSubset(m_ks, _T("PrOg1")));
		Assert::AreEqual(programsOnly, shortcut.isSubset(m_ks, _T("PROG1")));
		Assert::AreEqual(programsOnly, shortcut.isSubset(m_ks, _T("prog2")));
		Assert::AreEqual(programsOnly, shortcut.isSubset(m_ks, _T("prog3")));
		Assert::AreEqual(!programsOnly, shortcut.isSubset(m_ks, _T("prog1.exe")));
		Assert::AreEqual(!programsOnly, shortcut.isSubset(m_ks, _T("pro")));
		Assert::AreEqual(!programsOnly, shortcut.isSubset(m_ks, _T("ro")));
		Assert::AreEqual(!programsOnly, shortcut.isSubset(m_ks, _T("rog2")));
		Assert::AreEqual(!programsOnly, shortcut.isSubset(m_ks, _T("other")));
	}
	
private:
	
	Keystroke m_ks;
};

TEST_CLASS(ShortcutCompareTest) {
public:
	
	TEST_METHOD_INITIALIZE(setUp) {
		Keystroke ks;
		ks.m_vk = 'A';
		m_shortcut1 = new Shortcut(ks);
		m_shortcut2 = new Shortcut(ks);
	}
	
	TEST_METHOD_CLEANUP(tearDown) {
		delete m_shortcut1;
		delete m_shortcut2;
	}
	
	
	TEST_METHOD(Contents_bothAreCommands) {
		m_shortcut1->m_type = m_shortcut2->m_type = Shortcut::Type::kCommand;
		m_shortcut1->m_command = _T("AZZZZZ");
		m_shortcut2->m_command = _T("Baa");
		checkCompare(kColContents, -1);
	}
	
	TEST_METHOD(Contents_bothAreTexts) {
		m_shortcut1->m_type = m_shortcut2->m_type = Shortcut::Type::kText;
		m_shortcut1->m_text = _T("Baa");
		m_shortcut2->m_text = _T("AZZZZZ");
		checkCompare(kColContents, +1);
	}
	
	TEST_METHOD(Contents_commandAndText) {
		m_shortcut1->m_type = Shortcut::Type::kText;
		m_shortcut2->m_type = Shortcut::Type::kCommand;
		checkCompare(kColContents, -1);
	}
	
	
	TEST_METHOD(Keystroke_equal) {
		checkCompare(kColKeystroke, 0);
	}
	
	TEST_METHOD(Keystroke_noModCode) {
		m_shortcut1->m_vk = 'A';
		m_shortcut2->m_vk = 'B';
		checkCompare(kColKeystroke, -1);
	}
	
	TEST_METHOD(Keystroke_sameModCode) {
		m_shortcut1->m_vk = 'A';
		m_shortcut2->m_vk = 'B';
		m_shortcut1->m_sided_mod_code = m_shortcut2->m_sided_mod_code
				= MOD_ALT | MOD_SHIFT | MOD_CONTROL | MOD_WIN;
		checkCompare(kColKeystroke, -1);
	}
	
	TEST_METHOD(Keystroke_winControlShiftBeforeWinControlAlt) {
		m_shortcut1->m_sided_mod_code = MOD_WIN | MOD_CONTROL | MOD_SHIFT;
		m_shortcut2->m_sided_mod_code = MOD_WIN | MOD_CONTROL | MOD_ALT;
		checkCompare(kColKeystroke, -1);
	}
	
	
	TEST_METHOD(Cond_empty) {
		checkCompare(kColCond, 0);
	}
	
	TEST_METHOD(Cond_equal) {
		m_shortcut1->m_conditions[Keystroke::kCondTypeCapsLock] =
			m_shortcut2->m_conditions[Keystroke::kCondTypeCapsLock] =
				Keystroke::Condition::kYes;
		m_shortcut1->m_programs =
			m_shortcut2->m_programs = _T("same");
		checkCompare(kColCond, 0);
	}
	
	TEST_METHOD(Cond_differentCondition) {
		m_shortcut1->m_conditions[Keystroke::kCondTypeCapsLock] = Keystroke::Condition::kYes;
		m_shortcut2->m_conditions[Keystroke::kCondTypeNumLock] = Keystroke::Condition::kNo;
		checkCompare(kColCond, -1);
	}
	
	TEST_METHOD(Cond_differentPrograms) {
		m_shortcut1->m_programs = _T("aaa");
		m_shortcut2->m_programs = _T("bbb");
		checkCompare(kColCond, -1);
	}
	
	
	TEST_METHOD(Description_empty) {
		checkCompare(kColDescription, 0);
	}
	
	TEST_METHOD(Description_equal) {
		m_shortcut1->m_description = m_shortcut2->m_description = _T("same");
		checkCompare(kColDescription, 0);
	}
	
	TEST_METHOD(Description_different) {
		m_shortcut1->m_description = _T("Aaaa");
		m_shortcut2->m_description = _T("Bbbb");
		checkCompare(kColDescription, -1);
	}
	
	TEST_METHOD(Description_ignoreCase) {
		m_shortcut1->m_description = _T("AAAA");
		m_shortcut2->m_description = _T("aaaa");
		checkCompare(kColDescription, 0);
	}
	
private:
	
	Shortcut* m_shortcut1;
	Shortcut* m_shortcut2;
	
	void checkCompare(int sort_column, int expected_result) {
		Shortcut::s_sort_column = sort_column;
		Assert::AreEqual(+expected_result,
			testing::normalizeCompareResult(Shortcut::compare(m_shortcut1, m_shortcut2, 0)));
		Assert::AreEqual(-expected_result,
			testing::normalizeCompareResult(Shortcut::compare(m_shortcut2, m_shortcut1, 0)));
	}
};


TEST_CLASS(ShortcutListTest) {
public:
	
	TEST_METHOD_INITIALIZE(setUp) {
		i18n::setLanguage(i18n::kLangEN);
		shortcut::initialize();
	}
	
	TEST_METHOD_CLEANUP(tearDown) {
		for (Shortcut* sh = shortcut::getFirst(); sh; sh = sh->getNext()) {
			sh->unregisterHotKey();
		}
		shortcut::clearShortcuts();
		shortcut::terminate();
		*e_ini_filepath = _T('\0');
	}
	
	TEST_METHOD(InitiallyEmpty) {
		Assert::IsNull(shortcut::getFirst());
	}
	
	TEST_METHOD(AddToList_one) {
		Shortcut *const shortcut = createShortcut('A');
		
		shortcut->addToList();
		
		Assert::AreSame(*shortcut, *shortcut::getFirst());
		Assert::IsNull(shortcut->getNext());
	}
	
	TEST_METHOD(AddToList_three) {
		Shortcut *const shortcut1 = createShortcut('1');
		Shortcut *const shortcut2 = createShortcut('2');
		Shortcut *const shortcut3 = createShortcut('3');
		
		shortcut1->addToList();
		shortcut2->addToList();
		shortcut3->addToList();
		
		Assert::AreSame(*shortcut1, *shortcut::getFirst());
		Assert::AreSame(*shortcut2, *shortcut1->getNext());
		Assert::AreSame(*shortcut3, *shortcut2->getNext());
		Assert::IsNull(shortcut3->getNext());
	}
	
	TEST_METHOD(Find) {
		Keystroke ks_ctrlA;
		ks_ctrlA.m_vk = 'A';
		ks_ctrlA.m_sided_mod_code = MOD_CONTROL;
		
		Keystroke ks_ctrlB;
		ks_ctrlB.m_vk = 'B';
		ks_ctrlB.m_sided_mod_code = MOD_CONTROL;
		
		Shortcut *const shortcut_ctrlA_notProg1 = new Shortcut(ks_ctrlA);
		shortcut_ctrlA_notProg1->m_programs = _T("prog1");
		shortcut_ctrlA_notProg1->addToList();
		
		Shortcut *const shortcut_ctrlB_prog123 = new Shortcut(ks_ctrlB);
		shortcut_ctrlB_prog123->m_programs = _T("prog1;prog2;prog3");
		shortcut_ctrlB_prog123->m_programs_only = true;
		shortcut_ctrlB_prog123->addToList();
		
		Shortcut *const shortcut_ctrlA_notProg2 = new Shortcut(ks_ctrlA);
		shortcut_ctrlA_notProg2->m_programs = _T("prog2");
		shortcut_ctrlA_notProg2->addToList();
		
		Shortcut *const shortcut_ctrlA_prog1 = new Shortcut(ks_ctrlA);
		shortcut_ctrlA_prog1->m_programs = _T("prog1");
		shortcut_ctrlA_prog1->m_programs_only = true;
		shortcut_ctrlA_prog1->addToList();
		
		Shortcut *const shortcut_ctrlA_prog23 = new Shortcut(ks_ctrlA);
		shortcut_ctrlA_prog23->m_programs = _T("prog2;prog3");
		shortcut_ctrlA_prog23->m_programs_only = true;
		shortcut_ctrlA_prog23->addToList();
		
		Shortcut *const shortcut_ctrlA_prog2 = new Shortcut(ks_ctrlA);
		shortcut_ctrlA_prog2->m_programs = _T("prog2");
		shortcut_ctrlA_prog2->m_programs_only = true;
		shortcut_ctrlA_prog2->addToList();
		
		// No match.
		Assert::IsNull(shortcut::find(ks_ctrlB, /* program= */ _T("other")));
		
		// Subset match.
		Assert::AreSame(*shortcut_ctrlB_prog123, *shortcut::find(ks_ctrlB, /* program= */ _T("prog2")));
		
		// Precedence:
		// 1) first programs_only = true match
		Assert::AreSame(*shortcut_ctrlA_notProg1, *shortcut::find(ks_ctrlA, /* program= */ nullptr));
		Assert::AreSame(*shortcut_ctrlA_notProg1, *shortcut::find(ks_ctrlA, /* program= */ _T("other")));
		Assert::AreSame(*shortcut_ctrlA_prog1, *shortcut::find(ks_ctrlA, /* program= */ _T("prog1")));
		Assert::AreSame(*shortcut_ctrlA_prog23, *shortcut::find(ks_ctrlA, /* program= */ _T("prog3")));
		Assert::AreSame(*shortcut_ctrlA_prog23, *shortcut::find(ks_ctrlA, /* program= */ _T("prog2")));
		
		// 2) first programs_only = false match
		Assert::AreSame(*shortcut_ctrlA_notProg1, *shortcut::find(ks_ctrlA, /* program= */ _T("other")));
	}
	
	TEST_METHOD(LoadShortcuts_overwritesSettings) {
		setNonDefaultGlobalValues();
		
		createShortcut('1')->addToList();
		createShortcut('2')->addToList();
		
		// Load the test config.
		testing::getProjectDir(e_ini_filepath);
		PathAppend(e_ini_filepath, _T("test_config.ini"));
		shortcut::loadShortcuts();
		
		assertTestGlobalValues(i18n::kLangEN);
		Assert::AreEqual(4, getShortcutCount());
	}
	
	TEST_METHOD(LoadShortcuts_overwritesGlobalSettingsAppendsShortcuts) {
		setNonDefaultGlobalValues();
		
		createShortcut('1')->addToList();
		createShortcut('2')->addToList();
		
		// Merge the test config.
		TCHAR merged_ini_filepath[MAX_PATH];
		testing::getProjectDir(merged_ini_filepath);
		PathAppend(merged_ini_filepath, _T("test_config.ini"));
		shortcut::mergeShortcuts(merged_ini_filepath);
		
		assertTestGlobalValues(i18n::kLangEN);
		Assert::AreEqual(6, getShortcutCount());
	}
	
	TEST_METHOD(ClearShortcuts) {
		createShortcut('1')->addToList();
		createShortcut('2')->addToList();
		
		shortcut::clearShortcuts();
		
		Assert::IsNull(shortcut::getFirst());
	}
	
	TEST_METHOD(SaveShortcuts_allLanguages) {
		// Load the test config.
		testing::getProjectDir(e_ini_filepath);
		PathAppend(e_ini_filepath, _T("test_config.ini"));
		shortcut::loadShortcuts();
		
		// Save the config to a golden file for each language.
		for (int langi = 0; langi < i18n::kLangCount; langi++) {
			i18n::Language lang = static_cast<i18n::Language>(langi);
			i18n::setLanguage(lang);
			
			testing::getProjectDir(e_ini_filepath);
			PathAppend(e_ini_filepath, StringPrintf(_T("Goldens\\test_config_%s.ini"), getLanguageName(lang)));
			shortcut::saveShortcuts();
		}
		
		// Load the config of each language. Save it again to verify idempotency.
		for (int langi = 0; langi < i18n::kLangCount; langi++) {
			i18n::Language lang = static_cast<i18n::Language>(langi);
			setNonDefaultGlobalValues();
			
			testing::getProjectDir(e_ini_filepath);
			PathAppend(e_ini_filepath, StringPrintf(_T("Goldens\\test_config_%s.ini"), getLanguageName(lang)));
			shortcut::loadShortcuts();
			
			assertTestGlobalValues(lang);
			Assert::AreEqual(4, getShortcutCount());
			
			shortcut::saveShortcuts();
		}
	}
	
private:
	
	static Shortcut* createShortcut(BYTE vk) {
		Keystroke ks;
		ks.m_vk = vk;
		return new Shortcut(ks);
	}
	
	static int getShortcutCount() {
		int shortcut_count = 0;
		for (Shortcut* sh = shortcut::getFirst(); sh; sh = sh->getNext()) {
			shortcut_count++;
		}
		return shortcut_count;
	}
	
	static void setNonDefaultGlobalValues() {
		i18n::setLanguage(i18n::kLangFR);
		e_main_dialog_size.cx = -1;
		e_main_dialog_size.cy = -2;
		e_maximize_main_dialog = true;
		e_icon_visible = false;
		for (int col = 0; col < arrayLength(e_column_widths); col++) {
			e_column_widths[col] = -1;
		}
	}
	
	// Verifies that the global values match test_config.ini.
	static void assertTestGlobalValues(i18n::Language expected_lang) {
		Assert::AreEqual(expected_lang, i18n::getLanguage());
		Assert::AreEqual(863L, e_main_dialog_size.cx);
		Assert::AreEqual(490L, e_main_dialog_size.cy);
		Assert::AreEqual(false, e_maximize_main_dialog);
		Assert::AreEqual(true, e_icon_visible);
		constexpr int expected_column_widths[] = { 35, 20, 15, 10, -1 };
		Assert::AreEqual(arrayLength(e_column_widths), arrayLength(expected_column_widths));
		for (int col = 0; col < arrayLength(e_column_widths); col++) {
			Assert::AreEqual(expected_column_widths[col], e_column_widths[col]);
		}
	}
};

}  // namespace ShortcutTest
