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

#include "MyString.h"

class StringTest : public CxxTest::TestSuite {
private:
	
	const LPCTSTR m_test_buf;
	const LPCTSTR m_old_value_buf;
	const LPCTSTR m_replace_buf;
	String m_empty;
	String m_null;
	String m_test;
	String m_old_value;
	String m_mixed_case;
	String m_replace;
	
public:
	
	StringTest() : m_test_buf(_T("test")), m_old_value_buf(_T("old value")),
		m_replace_buf(_T("one word two words")) {
	}
	
	void setUp() {
		m_empty = m_test_buf;
		m_empty[0] = _T('\0');
		m_null.empty();
		m_test = m_test_buf;
		m_old_value = m_old_value_buf;
		m_mixed_case = _T("MiXeD cAsE");
		m_replace = m_replace_buf;
	}
	
	void tearDown() {
		m_empty.empty();
		m_null.empty();
		m_test.empty();
		m_old_value.empty();
		m_mixed_case.empty();
		m_replace.empty();
	}
	
	void checkStringEqual(LPCTSTR expected, LPCTSTR got) {
		char error_message[1024];
		wsprintfA(error_message, "Expected: \"%ws\", found: \"%ws\"", expected, got);
		TSM_ASSERT_EQUALS(error_message, 0, lstrcmp(expected, got));
	}
	
	//--------------------------------------------------------------------------------------------------
	// Construction, destruction
	//--------------------------------------------------------------------------------------------------
	
	void testConstructEmpty() {
		checkStringEqual(_T(""), String());
	}
	
	
	void testConstructAffect() {
		String str = m_test_buf;
		checkStringEqual(m_test_buf, str);
	}
	
	void testConstructAffect_empty() {
		String str = _T("");  // Use implicit constructor
		checkStringEqual(_T(""), str);
	}
	
	void testConstructAffect_null() {
		String str = (LPCTSTR)NULL;  // Use implicit constructor
		checkStringEqual(_T(""), str);
	}
	
	
	void testConstructAffectObject_other() {
		String str = m_test;  // Use implicit constructor
		checkStringEqual(m_test_buf, str);
	}
	
	void testConstructAffectObject_empty() {
		String str = m_empty;  // Use implicit constructor
		checkStringEqual(_T(""), str);
	}
	
	void testConstructAffectObject_null() {
		String str = m_null;  // Use implicit constructor
		checkStringEqual(_T(""), str);
	}
	
	//--------------------------------------------------------------------------------------------------
	// Getters
	//--------------------------------------------------------------------------------------------------
	
	void testGetCstr_sameString() {
		TS_ASSERT_EQUALS(m_test.get(), (LPCTSTR)m_test);
	}
	
	void testGetCstr_neverNull() {
		TS_ASSERT_EQUALS((LPCTSTR)NULL, m_null.get());
		TS_ASSERT_DIFFERS((LPCTSTR)NULL, (LPCTSTR)m_null);
	}
	
	void testGetCstr_emptyStringIfNull() {
		TS_ASSERT_EQUALS(_T('\0'), *(LPCTSTR)m_null);
	}
	
	
	void testGetSafe_sameString() {
		TS_ASSERT_EQUALS(m_test.get(), m_test.getSafe());
	}
	
	void testGetSafe_neverNull() {
		TS_ASSERT_EQUALS((LPCTSTR)NULL, m_null.get());
		TS_ASSERT_DIFFERS((LPCTSTR)NULL, m_null.getSafe());
	}
	
	void testGetSafe_emptyStringIfNull() {
		TS_ASSERT_EQUALS(_T('\0'), *m_null.getSafe());
	}
	
	void testGetSafe_hangsIfInstanceHasNullAddress() {
		String *pstring = NULL;
		TS_ASSERT_THROWS(pstring->getSafe(), testing::SeException);
	}
	
	
	void testAccessCharSigned_notNull() const {
		TS_ASSERT_EQUALS('s', m_test[(int)2]);
	}
	
	void testAccessCharUnsigned_notNull() const {
		TS_ASSERT_EQUALS(_T('s'), (TCHAR)m_test[(size_t)2]);
	}
	
	void testAccessCharSigned_null() const {
		TS_ASSERT_EQUALS(_T('\0'), (TCHAR)m_null[(int)0]);
	}
	
	void testAccessCharUnsigned_null() const {
		TS_ASSERT_EQUALS(_T('\0'), (TCHAR)m_null[(size_t)0]);
	}
	
	void testAccessCharSignedModify_notNull() {
		m_test[(int)2] = _T('X');
		checkStringEqual(m_test, _T("teXt"));
	}
	
	void testAccessCharUnsignedModify_notNull() {
		m_test[(size_t)2] = _T('X');
		checkStringEqual(m_test, _T("teXt"));
	}
	
	void testAccessCharSignedModify_nullFails() {
		TS_ASSERT_THROWS_ANYTHING(m_null[(int)0] = _T('X'));
	}
	
	void testAccessCharUnsignedModify_nullFails() {
		TS_ASSERT_THROWS_ANYTHING(m_null[(size_t)0] = _T('X'));
	}
	
	
	void testGetLength_empty() {
		TS_ASSERT_EQUALS(0, m_empty.getLength());
	}
	
	void testGetLength_null() {
		TS_ASSERT_EQUALS(0, m_null.getLength());
	}
	
	void testGetLength_notEmpty() {
		TS_ASSERT_EQUALS(4, m_test.getLength());
	}
	
	
	void testIsEmpty_whenEmpty() {
		TS_ASSERT_EQUALS(0, m_empty.getLength());
	}
	
	void testIsEmpty_whenNull() {
		TS_ASSERT_EQUALS(0, m_null.getLength());
	}
	
	//--------------------------------------------------------------------------------------------------
	// Affectation
	//--------------------------------------------------------------------------------------------------
	
	void testAffect_shorter() {
		m_old_value = m_test_buf;
		checkStringEqual(m_test_buf, m_old_value);
	}
	
	void testAffect_onger() {
		String str = m_old_value_buf;
		str = _T("test with a longer string");
		checkStringEqual(_T("test with a longer string"), str);
	}
	
	void testAffect_self() {
		m_test = (LPCTSTR)m_test;
		checkStringEqual(m_test_buf, m_test);
	}
	
	void testAffect_selfSuffix() {
		m_test = ((LPCTSTR)m_test) + 2;
		checkStringEqual(_T("st"), m_test);
	}
	
	void testAffect_empty() {
		m_test = _T("");
		checkStringEqual(_T(""), m_test);
	}
	
	void testAffect_null() {
		m_test = (LPCTSTR)NULL;
		checkStringEqual(_T(""), m_test);
	}
	
	
	void testAffectObject_other() {
		String str = m_old_value_buf;
		str = String(m_test_buf);
		checkStringEqual(m_test_buf, str);
	}
	
	void testAffectObject_keepsSourceUnchanged() {
		String sDest = m_old_value_buf;
		{
			String sSource = m_test_buf;
			sDest = sSource;
			checkStringEqual(m_test_buf, sSource);
		}
		checkStringEqual(m_test_buf, sDest);
	}
	
	void testAffectObject_self() {
		m_test = m_test;
		checkStringEqual(m_test_buf, m_test);
	}
	
	void testAffectObject_empty() {
		m_test = m_empty;
		checkStringEqual(_T(""), m_test);
	}
	
	//--------------------------------------------------------------------------------------------------
	// Appending
	//--------------------------------------------------------------------------------------------------
	
	void testAppend_notEmptyToNotEmpty() {
		m_test += _T(" more");
		checkStringEqual(_T("test more"), m_test);
	}
	
	void testAppend_toEmpty() {
		m_empty += m_test_buf;
		checkStringEqual(m_test_buf, m_empty);
	}
	
	void testAppend_toNull() {
		m_null += m_test_buf;
		checkStringEqual(m_test_buf, m_null);
	}
	
	void testAppend_empty() {
		m_test += _T("");
		checkStringEqual(m_test_buf, m_test);
	}
	
	void testAppend_null() {
		m_test += (LPCTSTR)NULL;
		checkStringEqual(m_test_buf, m_test);
	}
	
	void testAppend_returnsSelf() {
		String& resultRef = (m_test += _T("other"));
		TS_ASSERT_EQUALS(&m_test, &resultRef);
	}
	
	void testAppend_selfNotEmpty() {
		m_test += (LPCTSTR)m_test;
		checkStringEqual(_T("testtest"), m_test);
	}
	
	void testAppend_selfEmpty() {
		m_empty += (LPCTSTR)m_empty;
		checkStringEqual(_T(""), m_empty);
	}
	
	void testAppend_selfNull() {
		m_null += (LPCTSTR)m_null;
		checkStringEqual(_T(""), m_null);
	}
	
	void testAppend_selfSuffix() {
		m_test += ((LPCTSTR)m_test) + 2;
		checkStringEqual(_T("testst"), m_test);
	}
	
	
	void testAppendObject_notEmptyToNotEmpty() {
		m_test += String(_T(" more"));
		checkStringEqual(_T("test more"), m_test);
	}
	
	void testAppendObject_empty() {
		m_test += m_empty;
		checkStringEqual(m_test_buf, m_test);
	}
	
	void testAppendObject_selfEmpty() {
		m_empty += m_empty;
		checkStringEqual(_T(""), m_empty);
	}
	
	void testAppendObject_selfNotEmpty() {
		m_test += m_test;
		checkStringEqual(_T("testtest"), m_test);
	}
	
	void testAppendObject_returnsSelf() {
		String& resultRef = (m_test += m_test);
		TS_ASSERT_EQUALS(&m_test, &resultRef);
	}
	
	
	void testAppendChar() {
		m_test += _T('X');
		checkStringEqual(_T("testX"), m_test);
	}
	
	void testAppendChar_toEmpty() {
		m_empty += _T('X');
		checkStringEqual(_T("X"), m_empty);
	}
	
	void testAppendChar_toNull() {
		m_null += _T('X');
		checkStringEqual(_T("X"), m_null);
	}
	
	//--------------------------------------------------------------------------------------------------
	// Operations
	//--------------------------------------------------------------------------------------------------
	
	void testGetBuffer_noopIfNeedsSmaller() {
		const int bufLengthOld = m_test.getBufferSize();
		m_test.getBuffer(2);
		TS_ASSERT_EQUALS(bufLengthOld, m_test.getBufferSize());
	}
	
	void testGetBuffer_enlargeIfNeedsLarger() {
		m_test.getBuffer(100);
		TS_ASSERT_LESS_THAN_EQUALS(100, m_test.getBufferSize());
	}
	
	void testGetBuffer_returnsBuffer() {
		checkStringEqual(_T("test"), m_test.getBuffer(10));
	}
	
	//--------------------------------------------------------------------------------------------------
	// Memory management
	//--------------------------------------------------------------------------------------------------
	
	void testBufferAllocFree() {
		const LPTSTR strbuf = String::bufferAlloc(11);
		lstrcpy(strbuf, _T("0123456789"));
		String::bufferFree(strbuf);
	}
	
	void testBufferReallocFree() {
		LPTSTR strbuf = String::bufferAlloc(11);
		lstrcpy(strbuf, _T("0123456789"));
		strbuf = String::bufferRealloc(strbuf, 20);
		checkStringEqual(_T("0123456789"), strbuf);
		strbuf = String::bufferRealloc(strbuf, 5);
		strbuf[4] = _T('\0');
		checkStringEqual(_T("0123"), strbuf);
		String::bufferFree(strbuf);
	}
};
