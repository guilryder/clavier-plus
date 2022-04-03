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
#include "../Shortcut.h"


namespace Microsoft::VisualStudio::CppUnitTestFramework {

template<> inline std::wstring ToString<Shortcut>(const Shortcut& ks) {
	return std::wstring(StringPrintf(_T("Shortcut{m_vk=0x%02x ('%c')}"), ks.m_vk, ks.m_vk));
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
		shortcut::initialize();
	}
	
	TEST_METHOD_CLEANUP(tearDown) {
		shortcut::clearShortcuts();
		shortcut::terminate();
	}
	
	TEST_METHOD(InitiallyEmpty) {
		Assert::IsNull(shortcut::getFirst());
	}
	
	TEST_METHOD(AddToList_one) {
		Shortcut *const shortcut = createShortcut('A');
		
		shortcut->addToList();
		
		Assert::AreSame(*shortcut::getFirst(), *shortcut);
		Assert::IsNull(shortcut->getNext());
	}
	
	TEST_METHOD(AddToList_three) {
		Shortcut *const shortcut1 = createShortcut('1');
		Shortcut *const shortcut2 = createShortcut('2');
		Shortcut *const shortcut3 = createShortcut('3');
		
		shortcut1->addToList();
		shortcut2->addToList();
		shortcut3->addToList();
		
		Assert::AreSame(*shortcut::getFirst(), *shortcut1);
		Assert::AreSame(*shortcut1->getNext(), *shortcut2);
		Assert::AreSame(*shortcut2->getNext(), *shortcut3);
		Assert::IsNull(shortcut3->getNext());
	}
	
private:
	
	static Shortcut* createShortcut(BYTE vk) {
		Keystroke ks;
		ks.m_vk = vk;
		return new Shortcut(ks);
	}
};

}  // namespace ShortcutTest
