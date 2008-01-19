/* Generated file, do not edit */

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
//
// Test template for CxxUnit, instantiated for each test file.


#include "stdafx.h"

#ifndef CXXTEST_RUNNING
#define CXXTEST_RUNNING
#endif

#include <cxxtest/TestListener.h>
#include <cxxtest/TestTracker.h>
#include <cxxtest/TestRunner.h>
#include <cxxtest/RealDescriptions.h>

#define new DEBUG_NEW

#include "../ShortcutTest.hxx"

static ShortcutTest suite_ShortcutTest;

static CxxTest::List Tests_ShortcutTest = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_ShortcutTest( "../ShortcutTest.hxx", 21, "ShortcutTest", suite_ShortcutTest, Tests_ShortcutTest );

static class TestDescription_ShortcutTest_testSetMatchingLevel_getMatchingLevel : public CxxTest::RealTestDescription {
public:
 TestDescription_ShortcutTest_testSetMatchingLevel_getMatchingLevel() : CxxTest::RealTestDescription( Tests_ShortcutTest, suiteDescription_ShortcutTest, 39, "testSetMatchingLevel_getMatchingLevel" ) {}
 void runTest() { suite_ShortcutTest.testSetMatchingLevel_getMatchingLevel(); }
} testDescription_ShortcutTest_testSetMatchingLevel_getMatchingLevel;

static class TestDescription_ShortcutTest_testMatchProgram_noProgramsMismatch : public CxxTest::RealTestDescription {
public:
 TestDescription_ShortcutTest_testMatchProgram_noProgramsMismatch() : CxxTest::RealTestDescription( Tests_ShortcutTest, suiteDescription_ShortcutTest, 45, "testMatchProgram_noProgramsMismatch" ) {}
 void runTest() { suite_ShortcutTest.testMatchProgram_noProgramsMismatch(); }
} testDescription_ShortcutTest_testMatchProgram_noProgramsMismatch;

static class TestDescription_ShortcutTest_testMatchProgram_processNameOnly_match : public CxxTest::RealTestDescription {
public:
 TestDescription_ShortcutTest_testMatchProgram_processNameOnly_match() : CxxTest::RealTestDescription( Tests_ShortcutTest, suiteDescription_ShortcutTest, 51, "testMatchProgram_processNameOnly_match" ) {}
 void runTest() { suite_ShortcutTest.testMatchProgram_processNameOnly_match(); }
} testDescription_ShortcutTest_testMatchProgram_processNameOnly_match;

static class TestDescription_ShortcutTest_testMatchProgram_processNameOnly_mismatch : public CxxTest::RealTestDescription {
public:
 TestDescription_ShortcutTest_testMatchProgram_processNameOnly_mismatch() : CxxTest::RealTestDescription( Tests_ShortcutTest, suiteDescription_ShortcutTest, 56, "testMatchProgram_processNameOnly_mismatch" ) {}
 void runTest() { suite_ShortcutTest.testMatchProgram_processNameOnly_mismatch(); }
} testDescription_ShortcutTest_testMatchProgram_processNameOnly_mismatch;

static class TestDescription_ShortcutTest_testMatchProgram_processNameOnly_unknownProcessNameMatches : public CxxTest::RealTestDescription {
public:
 TestDescription_ShortcutTest_testMatchProgram_processNameOnly_unknownProcessNameMatches() : CxxTest::RealTestDescription( Tests_ShortcutTest, suiteDescription_ShortcutTest, 61, "testMatchProgram_processNameOnly_unknownProcessNameMatches" ) {}
 void runTest() { suite_ShortcutTest.testMatchProgram_processNameOnly_unknownProcessNameMatches(); }
} testDescription_ShortcutTest_testMatchProgram_processNameOnly_unknownProcessNameMatches;

static class TestDescription_ShortcutTest_testMatchProgram_processNameOnly_caseDoesNotMatter : public CxxTest::RealTestDescription {
public:
 TestDescription_ShortcutTest_testMatchProgram_processNameOnly_caseDoesNotMatter() : CxxTest::RealTestDescription( Tests_ShortcutTest, suiteDescription_ShortcutTest, 66, "testMatchProgram_processNameOnly_caseDoesNotMatter" ) {}
 void runTest() { suite_ShortcutTest.testMatchProgram_processNameOnly_caseDoesNotMatter(); }
} testDescription_ShortcutTest_testMatchProgram_processNameOnly_caseDoesNotMatter;

static class TestDescription_ShortcutTest_testMatchProgram_windowTitleOnlyNoRegexp_match : public CxxTest::RealTestDescription {
public:
 TestDescription_ShortcutTest_testMatchProgram_windowTitleOnlyNoRegexp_match() : CxxTest::RealTestDescription( Tests_ShortcutTest, suiteDescription_ShortcutTest, 73, "testMatchProgram_windowTitleOnlyNoRegexp_match" ) {}
 void runTest() { suite_ShortcutTest.testMatchProgram_windowTitleOnlyNoRegexp_match(); }
} testDescription_ShortcutTest_testMatchProgram_windowTitleOnlyNoRegexp_match;

static class TestDescription_ShortcutTest_testMatchProgram_windowTitleOnlyNoRegexp_mismatch : public CxxTest::RealTestDescription {
public:
 TestDescription_ShortcutTest_testMatchProgram_windowTitleOnlyNoRegexp_mismatch() : CxxTest::RealTestDescription( Tests_ShortcutTest, suiteDescription_ShortcutTest, 78, "testMatchProgram_windowTitleOnlyNoRegexp_mismatch" ) {}
 void runTest() { suite_ShortcutTest.testMatchProgram_windowTitleOnlyNoRegexp_mismatch(); }
} testDescription_ShortcutTest_testMatchProgram_windowTitleOnlyNoRegexp_mismatch;

static class TestDescription_ShortcutTest_testMatchProgram_windowTitleOnlyRegexp_match : public CxxTest::RealTestDescription {
public:
 TestDescription_ShortcutTest_testMatchProgram_windowTitleOnlyRegexp_match() : CxxTest::RealTestDescription( Tests_ShortcutTest, suiteDescription_ShortcutTest, 83, "testMatchProgram_windowTitleOnlyRegexp_match" ) {}
 void runTest() { suite_ShortcutTest.testMatchProgram_windowTitleOnlyRegexp_match(); }
} testDescription_ShortcutTest_testMatchProgram_windowTitleOnlyRegexp_match;

static class TestDescription_ShortcutTest_testMatchProgram_windowTitleOnlyRegexp_mismatch : public CxxTest::RealTestDescription {
public:
 TestDescription_ShortcutTest_testMatchProgram_windowTitleOnlyRegexp_mismatch() : CxxTest::RealTestDescription( Tests_ShortcutTest, suiteDescription_ShortcutTest, 88, "testMatchProgram_windowTitleOnlyRegexp_mismatch" ) {}
 void runTest() { suite_ShortcutTest.testMatchProgram_windowTitleOnlyRegexp_mismatch(); }
} testDescription_ShortcutTest_testMatchProgram_windowTitleOnlyRegexp_mismatch;

static class TestDescription_ShortcutTest_testMatchProgram_windowTitleOnlyRegexp_emptyTitleMismatches : public CxxTest::RealTestDescription {
public:
 TestDescription_ShortcutTest_testMatchProgram_windowTitleOnlyRegexp_emptyTitleMismatches() : CxxTest::RealTestDescription( Tests_ShortcutTest, suiteDescription_ShortcutTest, 93, "testMatchProgram_windowTitleOnlyRegexp_emptyTitleMismatches" ) {}
 void runTest() { suite_ShortcutTest.testMatchProgram_windowTitleOnlyRegexp_emptyTitleMismatches(); }
} testDescription_ShortcutTest_testMatchProgram_windowTitleOnlyRegexp_emptyTitleMismatches;

static class TestDescription_ShortcutTest_testMatchProgram_windowTitleOnly_caseMatters : public CxxTest::RealTestDescription {
public:
 TestDescription_ShortcutTest_testMatchProgram_windowTitleOnly_caseMatters() : CxxTest::RealTestDescription( Tests_ShortcutTest, suiteDescription_ShortcutTest, 98, "testMatchProgram_windowTitleOnly_caseMatters" ) {}
 void runTest() { suite_ShortcutTest.testMatchProgram_windowTitleOnly_caseMatters(); }
} testDescription_ShortcutTest_testMatchProgram_windowTitleOnly_caseMatters;

static class TestDescription_ShortcutTest_testMatchProgram_processAndTitle_match : public CxxTest::RealTestDescription {
public:
 TestDescription_ShortcutTest_testMatchProgram_processAndTitle_match() : CxxTest::RealTestDescription( Tests_ShortcutTest, suiteDescription_ShortcutTest, 105, "testMatchProgram_processAndTitle_match" ) {}
 void runTest() { suite_ShortcutTest.testMatchProgram_processAndTitle_match(); }
} testDescription_ShortcutTest_testMatchProgram_processAndTitle_match;

static class TestDescription_ShortcutTest_testMatchProgram_processAndTitle_mismatch : public CxxTest::RealTestDescription {
public:
 TestDescription_ShortcutTest_testMatchProgram_processAndTitle_mismatch() : CxxTest::RealTestDescription( Tests_ShortcutTest, suiteDescription_ShortcutTest, 111, "testMatchProgram_processAndTitle_mismatch" ) {}
 void runTest() { suite_ShortcutTest.testMatchProgram_processAndTitle_mismatch(); }
} testDescription_ShortcutTest_testMatchProgram_processAndTitle_mismatch;

static class TestDescription_ShortcutTest_testMatchProgram_processAndTitle_lastProgramMatches : public CxxTest::RealTestDescription {
public:
 TestDescription_ShortcutTest_testMatchProgram_processAndTitle_lastProgramMatches() : CxxTest::RealTestDescription( Tests_ShortcutTest, suiteDescription_ShortcutTest, 118, "testMatchProgram_processAndTitle_lastProgramMatches" ) {}
 void runTest() { suite_ShortcutTest.testMatchProgram_processAndTitle_lastProgramMatches(); }
} testDescription_ShortcutTest_testMatchProgram_processAndTitle_lastProgramMatches;

