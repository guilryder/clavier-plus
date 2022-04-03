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
#include "../MyString.h"

namespace MyStringTest {

TEST_CLASS(StringTest) {
public:
	
	StringTest() : m_test_buf(_T("test")), m_old_value_buf(_T("old value")),
		m_replace_buf(_T("one word two words")) {
	}
	
	TEST_METHOD_INITIALIZE(setUp) {
		m_empty = m_test_buf;
		m_empty[0] = _T('\0');
		m_null.empty();
		m_test = m_test_buf;
		m_old_value = m_old_value_buf;
		m_mixed_case = _T("MiXeD cAsE");
		m_replace = m_replace_buf;
	}
	
	TEST_METHOD_CLEANUP(tearDown) {
		m_empty.empty();
		m_null.empty();
		m_test.empty();
		m_old_value.empty();
		m_mixed_case.empty();
		m_replace.empty();
	}
	
	//--------------------------------------------------------------------------------------------------
	// Construction, destruction
	//--------------------------------------------------------------------------------------------------
	
	TEST_METHOD(ConstructorEmpty) {
		Assert::AreEqual(_T(""), String());
	}
	
	
	TEST_METHOD(ConstructorBuffer_notEmpty) {
		String str = m_test_buf;
		Assert::AreEqual(m_test_buf, str);
	}
	
	TEST_METHOD(ConstructorBuffer_empty) {
		String str = _T("");  // Use implicit constructor
		Assert::AreEqual(_T(""), str);
	}
	
	TEST_METHOD(ConstructorBuffer_null) {
		String str = static_cast<LPCTSTR>(nullptr);  // Use implicit constructor
		Assert::AreEqual(_T(""), str);
	}
	
	
	TEST_METHOD(ConstructorBufferLength_exactLength) {
		LPCTSTR input = _T("12345");
		String str(input, 5);
		Assert::AreEqual(input, str);
	}
	
	TEST_METHOD(ConstructorBufferLength_shorterLength) {
		LPCTSTR input = _T("1234");
		String str(input, 3);
		Assert::AreEqual(_T("123"), str);
	}
	
	TEST_METHOD(ConstructorBufferLength_negativeLength) {
		String str(m_test_buf, -1);
		Assert::AreEqual(_T(""), str);
	}
	
	TEST_METHOD(ConstructorBufferLength_empty) {
		String str(_T(""), 0);
		Assert::AreEqual(_T(""), str);
	}
	
	TEST_METHOD(ConstructorBufferLength_null) {
		String str(static_cast<LPCTSTR>(nullptr), 0);
		Assert::AreEqual(_T(""), str);
	}
	
	
	TEST_METHOD(ConstructorCopy_other) {
		String str = m_test;  // Use implicit constructor
		Assert::AreEqual(m_test_buf, str);
	}
	
	TEST_METHOD(ConstructorCopy_empty) {
		String str = m_empty;  // Use implicit constructor
		Assert::AreEqual(_T(""), str);
	}
	
	TEST_METHOD(ConstructorCopy_null) {
		String str = m_null;  // Use implicit constructor
		Assert::AreEqual(_T(""), str);
	}
	
	
	TEST_METHOD(ConstructorMove_nonEmpty) {
		String source = m_test_buf;
		LPTSTR source_ptr = source.get();
		String dest = std::move(source);
		Assert::AreEqual(_T(""), source);
		Assert::AreEqual(m_test_buf, dest);
		Assert::AreSame(*source_ptr, *dest.get());
	}
	
	TEST_METHOD(ConstructorMove_empty) {
		String source = m_empty;
		String dest = std::move(source);
		Assert::AreEqual(_T(""), source);
		Assert::AreEqual(_T(""), dest);
	}
	
	TEST_METHOD(ConstructorMove_null) {
		String source = m_null;
		String dest = std::move(source);
		Assert::AreEqual(_T(""), source);
		Assert::AreEqual(_T(""), dest);
	}
	
	//--------------------------------------------------------------------------------------------------
	// Getters
	//--------------------------------------------------------------------------------------------------
	
	TEST_METHOD(GetCstr_sameString) {
		Assert::AreEqual(m_test.get(), static_cast<LPCTSTR>(m_test));
	}
	
	TEST_METHOD(GetCstr_neverNull) {
		Assert::IsNull(m_null.get());
		Assert::IsNotNull(static_cast<LPCTSTR>(m_null));
	}
	
	TEST_METHOD(GetCstr_emptyStringIfNull) {
		Assert::AreEqual(_T('\0'), *static_cast<LPCTSTR>(m_null));
	}
	
	
	TEST_METHOD(GetSafe_sameString) {
		Assert::AreEqual(m_test.get(), m_test.getSafe());
	}
	
	TEST_METHOD(GetSafe_neverNull) {
		Assert::IsNull(m_null.get());
		Assert::IsNotNull(m_null.getSafe());
	}
	
	TEST_METHOD(GetSafe_emptyStringIfNull) {
		Assert::AreEqual(_T('\0'), *m_null.getSafe());
	}
	
	TEST_METHOD(GetSafe_hangsIfInstanceHasNullAddress) {
		String *string_ptr = nullptr;
		Assert::ExpectException<std::runtime_error>([&]{ string_ptr->getSafe(); });
	}
	
	
	TEST_METHOD(AccessCharSigned_notNull) {
		Assert::AreEqual(_T('s'), m_test[int(2)]);
	}
	
	TEST_METHOD(AccessCharUnsigned_notNull) {
		Assert::AreEqual(_T('s'), m_test[size_t(2)]);
	}
	
	TEST_METHOD(AccessCharSigned_null) {
		const String& const_null = m_null;
		Assert::AreEqual(_T('\0'), const_null[int(0)]);
	}
	
	TEST_METHOD(AccessCharUnsigned_null) {
		const String& const_null = m_null;
		Assert::AreEqual(_T('\0'), const_null[size_t(0)]);
	}
	
	TEST_METHOD(AccessCharSignedModify_notNull) {
		m_test[int(2)] = _T('X');
		Assert::AreEqual(m_test, _T("teXt"));
	}
	
	TEST_METHOD(AccessCharUnsignedModify_notNull) {
		m_test[size_t(2)] = _T('X');
		Assert::AreEqual(m_test, _T("teXt"));
	}
	
	TEST_METHOD(AccessCharSignedModify_nullFails) {
		Assert::ExpectException<std::exception>([&]{ m_null[int(0)] = _T('X'); });
	}
	
	TEST_METHOD(AccessCharUnsignedModify_nullFails) {
		Assert::ExpectException<std::exception>([&]{ m_null[size_t(0)] = _T('X'); });
	}
	
	
	TEST_METHOD(GetLength_empty) {
		Assert::AreEqual(0, m_empty.getLength());
	}
	
	TEST_METHOD(GetLength_null) {
		Assert::AreEqual(0, m_null.getLength());
	}
	
	TEST_METHOD(GetLength_notEmpty) {
		Assert::AreEqual(4, m_test.getLength());
	}
	
	
	TEST_METHOD(IsEmpty_whenEmpty) {
		Assert::AreEqual(0, m_empty.getLength());
	}
	
	TEST_METHOD(IsEmpty_whenNull) {
		Assert::AreEqual(0, m_null.getLength());
	}
	
	//--------------------------------------------------------------------------------------------------
	// Affectation
	//--------------------------------------------------------------------------------------------------
	
	TEST_METHOD(Affect_shorter) {
		m_old_value = m_test_buf;
		Assert::AreEqual(m_test_buf, m_old_value);
	}
	
	TEST_METHOD(Affect_onger) {
		String str = m_old_value_buf;
		str = _T("test with a longer string");
		Assert::AreEqual(_T("test with a longer string"), str);
	}
	
	TEST_METHOD(Affect_self) {
		m_test = static_cast<LPCTSTR>(m_test);
		Assert::AreEqual(m_test_buf, m_test);
	}
	
	TEST_METHOD(Affect_selfSuffix) {
		m_test = static_cast<LPCTSTR>(m_test) + 2;
		Assert::AreEqual(_T("st"), m_test);
	}
	
	TEST_METHOD(Affect_empty) {
		m_test = _T("");
		Assert::AreEqual(_T(""), m_test);
	}
	
	TEST_METHOD(Affect_null) {
		m_test = static_cast<LPCTSTR>(nullptr);
		Assert::AreEqual(_T(""), m_test);
	}
	
	
	TEST_METHOD(AffectObject_other) {
		String str = m_old_value_buf;
		str = String(m_test_buf);
		Assert::AreEqual(m_test_buf, str);
	}
	
	TEST_METHOD(AffectObject_keepsSourceUnchanged) {
		String sDest = m_old_value_buf;
		{
			String sSource = m_test_buf;
			sDest = sSource;
			Assert::AreEqual(m_test_buf, sSource);
		}
		Assert::AreEqual(m_test_buf, sDest);
	}
	
	TEST_METHOD(AffectObject_self) {
		m_test = m_test;
		Assert::AreEqual(m_test_buf, m_test);
	}
	
	TEST_METHOD(AffectObject_empty) {
		m_test = m_empty;
		Assert::AreEqual(_T(""), m_test);
	}
	
	//--------------------------------------------------------------------------------------------------
	// Appending
	//--------------------------------------------------------------------------------------------------
	
	TEST_METHOD(Append_notEmptyToNotEmpty) {
		m_test += _T(" more");
		Assert::AreEqual(_T("test more"), m_test);
	}
	
	TEST_METHOD(Append_toEmpty) {
		m_empty += m_test_buf;
		Assert::AreEqual(m_test_buf, m_empty);
	}
	
	TEST_METHOD(Append_toNull) {
		m_null += m_test_buf;
		Assert::AreEqual(m_test_buf, m_null);
	}
	
	TEST_METHOD(Append_empty) {
		m_test += _T("");
		Assert::AreEqual(m_test_buf, m_test);
	}
	
	TEST_METHOD(Append_null) {
		m_test += static_cast<LPCTSTR>(nullptr);
		Assert::AreEqual(m_test_buf, m_test);
	}
	
	TEST_METHOD(Append_returnsSelf) {
		String& resultRef = (m_test += _T("other"));
		Assert::AreSame(m_test, resultRef);
	}
	
	TEST_METHOD(Append_selfNotEmpty) {
		m_test += static_cast<LPCTSTR>(m_test);
		Assert::AreEqual(_T("testtest"), m_test);
	}
	
	TEST_METHOD(Append_selfEmpty) {
		m_empty += static_cast<LPCTSTR>(m_empty);
		Assert::AreEqual(_T(""), m_empty);
	}
	
	TEST_METHOD(Append_selfNull) {
		m_null += static_cast<LPCTSTR>(m_null);
		Assert::AreEqual(_T(""), m_null);
	}
	
	TEST_METHOD(Append_selfSuffix) {
		m_test += static_cast<LPCTSTR>(m_test) + 2;
		Assert::AreEqual(_T("testst"), m_test);
	}
	
	
	TEST_METHOD(AppendObject_notEmptyToNotEmpty) {
		m_test += String(_T(" more"));
		Assert::AreEqual(_T("test more"), m_test);
	}
	
	TEST_METHOD(AppendObject_empty) {
		m_test += m_empty;
		Assert::AreEqual(m_test_buf, m_test);
	}
	
	TEST_METHOD(AppendObject_selfEmpty) {
		m_empty += m_empty;
		Assert::AreEqual(_T(""), m_empty);
	}
	
	TEST_METHOD(AppendObject_selfNotEmpty) {
		m_test += m_test;
		Assert::AreEqual(_T("testtest"), m_test);
	}
	
	TEST_METHOD(AppendObject_returnsSelf) {
		String& resultRef = (m_test += m_test);
		Assert::AreSame(m_test, resultRef);
	}
	
	
	TEST_METHOD(AppendChar) {
		m_test += _T('X');
		Assert::AreEqual(_T("testX"), m_test);
	}
	
	TEST_METHOD(AppendChar_toEmpty) {
		m_empty += _T('X');
		Assert::AreEqual(_T("X"), m_empty);
	}
	
	TEST_METHOD(AppendChar_toNull) {
		m_null += _T('X');
		Assert::AreEqual(_T("X"), m_null);
	}
	
	//--------------------------------------------------------------------------------------------------
	// Operations
	//--------------------------------------------------------------------------------------------------
	
	TEST_METHOD(GetBuffer_noopIfNeedsSmaller) {
		const int bufLengthOld = m_test.getBufferSize();
		m_test.getBuffer(2);
		Assert::AreEqual(bufLengthOld, m_test.getBufferSize());
	}
	
	TEST_METHOD(GetBuffer_enlargeIfNeedsLarger) {
		m_test.getBuffer(100);
		Assert::IsTrue(m_test.getBufferSize() >= 100);
	}
	
	TEST_METHOD(GetBuffer_returnsBuffer) {
		Assert::AreEqual(_T("test"), m_test.getBuffer(10));
	}
	
	//--------------------------------------------------------------------------------------------------
	// Memory management
	//--------------------------------------------------------------------------------------------------
	
	TEST_METHOD(BufferAllocFree) {
		const LPTSTR strbuf = String::bufferAlloc(11);
		lstrcpy(strbuf, _T("0123456789"));
		String::bufferFree(strbuf);
	}
	
	TEST_METHOD(BufferReallocFree) {
		LPTSTR strbuf = String::bufferAlloc(11);
		lstrcpy(strbuf, _T("0123456789"));
		strbuf = String::bufferRealloc(strbuf, 20);
		Assert::AreEqual(_T("0123456789"), strbuf);
		strbuf = String::bufferRealloc(strbuf, 5);
		strbuf[4] = _T('\0');
		Assert::AreEqual(_T("0123"), strbuf);
		String::bufferFree(strbuf);
	}
	
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
};

}  // namespace MyStringTest
