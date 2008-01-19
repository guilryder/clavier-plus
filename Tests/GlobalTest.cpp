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

#include "../GlobalTest.hxx"

static MatchWildcardsTest suite_MatchWildcardsTest;

static CxxTest::List Tests_MatchWildcardsTest = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_MatchWildcardsTest( "../GlobalTest.hxx", 21, "MatchWildcardsTest", suite_MatchWildcardsTest, Tests_MatchWildcardsTest );

static class TestDescription_MatchWildcardsTest_testEmpty_matches_empty : public CxxTest::RealTestDescription {
public:
 TestDescription_MatchWildcardsTest_testEmpty_matches_empty() : CxxTest::RealTestDescription( Tests_MatchWildcardsTest, suiteDescription_MatchWildcardsTest, 24, "testEmpty_matches_empty" ) {}
 void runTest() { suite_MatchWildcardsTest.testEmpty_matches_empty(); }
} testDescription_MatchWildcardsTest_testEmpty_matches_empty;

static class TestDescription_MatchWildcardsTest_testEmpty_mismatches_notEmpty : public CxxTest::RealTestDescription {
public:
 TestDescription_MatchWildcardsTest_testEmpty_mismatches_notEmpty() : CxxTest::RealTestDescription( Tests_MatchWildcardsTest, suiteDescription_MatchWildcardsTest, 28, "testEmpty_mismatches_notEmpty" ) {}
 void runTest() { suite_MatchWildcardsTest.testEmpty_mismatches_notEmpty(); }
} testDescription_MatchWildcardsTest_testEmpty_mismatches_notEmpty;

static class TestDescription_MatchWildcardsTest_testSameString_matches : public CxxTest::RealTestDescription {
public:
 TestDescription_MatchWildcardsTest_testSameString_matches() : CxxTest::RealTestDescription( Tests_MatchWildcardsTest, suiteDescription_MatchWildcardsTest, 32, "testSameString_matches" ) {}
 void runTest() { suite_MatchWildcardsTest.testSameString_matches(); }
} testDescription_MatchWildcardsTest_testSameString_matches;

static class TestDescription_MatchWildcardsTest_testCaseMatters : public CxxTest::RealTestDescription {
public:
 TestDescription_MatchWildcardsTest_testCaseMatters() : CxxTest::RealTestDescription( Tests_MatchWildcardsTest, suiteDescription_MatchWildcardsTest, 36, "testCaseMatters" ) {}
 void runTest() { suite_MatchWildcardsTest.testCaseMatters(); }
} testDescription_MatchWildcardsTest_testCaseMatters;

static class TestDescription_MatchWildcardsTest_testQuestionMark_one : public CxxTest::RealTestDescription {
public:
 TestDescription_MatchWildcardsTest_testQuestionMark_one() : CxxTest::RealTestDescription( Tests_MatchWildcardsTest, suiteDescription_MatchWildcardsTest, 41, "testQuestionMark_one" ) {}
 void runTest() { suite_MatchWildcardsTest.testQuestionMark_one(); }
} testDescription_MatchWildcardsTest_testQuestionMark_one;

static class TestDescription_MatchWildcardsTest_testQuestionMark_two : public CxxTest::RealTestDescription {
public:
 TestDescription_MatchWildcardsTest_testQuestionMark_two() : CxxTest::RealTestDescription( Tests_MatchWildcardsTest, suiteDescription_MatchWildcardsTest, 45, "testQuestionMark_two" ) {}
 void runTest() { suite_MatchWildcardsTest.testQuestionMark_two(); }
} testDescription_MatchWildcardsTest_testQuestionMark_two;

static class TestDescription_MatchWildcardsTest_testQuestionMark_needsOtherCharacterSame : public CxxTest::RealTestDescription {
public:
 TestDescription_MatchWildcardsTest_testQuestionMark_needsOtherCharacterSame() : CxxTest::RealTestDescription( Tests_MatchWildcardsTest, suiteDescription_MatchWildcardsTest, 49, "testQuestionMark_needsOtherCharacterSame" ) {}
 void runTest() { suite_MatchWildcardsTest.testQuestionMark_needsOtherCharacterSame(); }
} testDescription_MatchWildcardsTest_testQuestionMark_needsOtherCharacterSame;

static class TestDescription_MatchWildcardsTest_testQuestionMark_needsSameLength : public CxxTest::RealTestDescription {
public:
 TestDescription_MatchWildcardsTest_testQuestionMark_needsSameLength() : CxxTest::RealTestDescription( Tests_MatchWildcardsTest, suiteDescription_MatchWildcardsTest, 53, "testQuestionMark_needsSameLength" ) {}
 void runTest() { suite_MatchWildcardsTest.testQuestionMark_needsSameLength(); }
} testDescription_MatchWildcardsTest_testQuestionMark_needsSameLength;

static class TestDescription_MatchWildcardsTest_testStart_atTheBeginning : public CxxTest::RealTestDescription {
public:
 TestDescription_MatchWildcardsTest_testStart_atTheBeginning() : CxxTest::RealTestDescription( Tests_MatchWildcardsTest, suiteDescription_MatchWildcardsTest, 59, "testStart_atTheBeginning" ) {}
 void runTest() { suite_MatchWildcardsTest.testStart_atTheBeginning(); }
} testDescription_MatchWildcardsTest_testStart_atTheBeginning;

static class TestDescription_MatchWildcardsTest_testStar_atTheEnd : public CxxTest::RealTestDescription {
public:
 TestDescription_MatchWildcardsTest_testStar_atTheEnd() : CxxTest::RealTestDescription( Tests_MatchWildcardsTest, suiteDescription_MatchWildcardsTest, 64, "testStar_atTheEnd" ) {}
 void runTest() { suite_MatchWildcardsTest.testStar_atTheEnd(); }
} testDescription_MatchWildcardsTest_testStar_atTheEnd;

static class TestDescription_MatchWildcardsTest_testStar_beginningAndEnd : public CxxTest::RealTestDescription {
public:
 TestDescription_MatchWildcardsTest_testStar_beginningAndEnd() : CxxTest::RealTestDescription( Tests_MatchWildcardsTest, suiteDescription_MatchWildcardsTest, 69, "testStar_beginningAndEnd" ) {}
 void runTest() { suite_MatchWildcardsTest.testStar_beginningAndEnd(); }
} testDescription_MatchWildcardsTest_testStar_beginningAndEnd;

static class TestDescription_MatchWildcardsTest_testStar_middle : public CxxTest::RealTestDescription {
public:
 TestDescription_MatchWildcardsTest_testStar_middle() : CxxTest::RealTestDescription( Tests_MatchWildcardsTest, suiteDescription_MatchWildcardsTest, 75, "testStar_middle" ) {}
 void runTest() { suite_MatchWildcardsTest.testStar_middle(); }
} testDescription_MatchWildcardsTest_testStar_middle;

static class TestDescription_MatchWildcardsTest_testPatternEnd_noWildcards : public CxxTest::RealTestDescription {
public:
 TestDescription_MatchWildcardsTest_testPatternEnd_noWildcards() : CxxTest::RealTestDescription( Tests_MatchWildcardsTest, suiteDescription_MatchWildcardsTest, 81, "testPatternEnd_noWildcards" ) {}
 void runTest() { suite_MatchWildcardsTest.testPatternEnd_noWildcards(); }
} testDescription_MatchWildcardsTest_testPatternEnd_noWildcards;

static class TestDescription_MatchWildcardsTest_testPatternEnd_withStar : public CxxTest::RealTestDescription {
public:
 TestDescription_MatchWildcardsTest_testPatternEnd_withStar() : CxxTest::RealTestDescription( Tests_MatchWildcardsTest, suiteDescription_MatchWildcardsTest, 87, "testPatternEnd_withStar" ) {}
 void runTest() { suite_MatchWildcardsTest.testPatternEnd_withStar(); }
} testDescription_MatchWildcardsTest_testPatternEnd_withStar;

static class TestDescription_MatchWildcardsTest_testPatternEnd_withStarAndQuestionMark : public CxxTest::RealTestDescription {
public:
 TestDescription_MatchWildcardsTest_testPatternEnd_withStarAndQuestionMark() : CxxTest::RealTestDescription( Tests_MatchWildcardsTest, suiteDescription_MatchWildcardsTest, 93, "testPatternEnd_withStarAndQuestionMark" ) {}
 void runTest() { suite_MatchWildcardsTest.testPatternEnd_withStarAndQuestionMark(); }
} testDescription_MatchWildcardsTest_testPatternEnd_withStarAndQuestionMark;

