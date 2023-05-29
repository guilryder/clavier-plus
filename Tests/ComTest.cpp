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
#include "../Com.h"

#include <msi.h>

namespace ComTest {

TEST_CLASS(CoPtrTest) {
public:
	
	TEST_METHOD(constructorEmpty) {
		CoPtr<IShellLink> ptr;
		Assert::IsFalse(ptr);
	}
	
	TEST_METHOD(constructorCreateInstance) {
		CoPtr<IShellLink> ptr(CLSID_ShellLink);
		Assert::IsTrue(ptr);
		assertOneReference(ptr);
	}
	
	TEST_METHOD(constructorMove) {
		CoPtr<IShellLink> ptr1(CLSID_ShellLink);
		IShellLink* raw_ptr = ptr1.get();
		
		CoPtr<IShellLink> ptr2 = std::move(ptr1);
		
		Assert::IsFalse(ptr1);
		Assert::IsTrue(ptr2);
		Assert::AreEqual(raw_ptr, ptr2.get());
		assertOneReference(ptr2);
	}
	
	TEST_METHOD(assignmentMove_validToNull) {
		CoPtr<IShellLink> ptr1(CLSID_ShellLink);
		IShellLink* raw_ptr = ptr1.get();
		
		CoPtr<IShellLink> ptr2;
		ptr2 = std::move(ptr1);
		
		Assert::IsFalse(ptr1);
		Assert::IsTrue(ptr2);
		Assert::AreEqual(raw_ptr, ptr2.get());
		assertOneReference(ptr2);
	}
	
	TEST_METHOD(assignmentMove_nullToValid) {
		CoPtr<IShellLink> ptr1;
		
		CoPtr<IShellLink> ptr2(CLSID_ShellLink);
		IShellLink* raw_ptr = ptr2.get();
		raw_ptr->AddRef();
		
		ptr2 = std::move(ptr1);
		
		Assert::IsFalse(ptr1);
		Assert::IsFalse(ptr2);
		Assert::AreEqual(0LU, raw_ptr->Release());
	}
	
	TEST_METHOD(assignmentMove_validToSelf) {
		CoPtr<IShellLink> ptr(CLSID_ShellLink);
		IShellLink* raw_ptr = ptr.get();
		
		ptr = std::move(ptr);
		
		Assert::AreEqual(raw_ptr, ptr.get());
		assertOneReference(ptr);
	}
	
	TEST_METHOD(assignmentMove_nullToNull) {
		CoPtr<IShellLink> ptr1;
		
		CoPtr<IShellLink> ptr2;
		ptr2 = std::move(ptr1);
		
		Assert::IsFalse(ptr1);
		Assert::IsFalse(ptr2);
	}
	
	TEST_METHOD(destructor_releases) {
		IShellLink* raw_ptr;
		{
			CoPtr<IShellLink> ptr(CLSID_ShellLink);
			raw_ptr = ptr.get();
			Assert::AreEqual(2LU, raw_ptr->AddRef());
		}
		Assert::AreEqual(0LU, raw_ptr->Release());
	}
	
	TEST_METHOD(arrowOperator_nullFails) {
		CoPtr<IShellLink> ptr;
		Assert::ExpectException<std::runtime_error>([&] {
			ptr->AddRef();
		});
	}
	
	TEST_METHOD(get_validPtr) {
		CoPtr<IShellLink> ptr(CLSID_ShellLink);
		IShellLink* raw_ptr = ptr.get();
		Assert::IsNotNull(raw_ptr);
		assertOneReference(raw_ptr);
	}
	
	TEST_METHOD(get_nullFails) {
		CoPtr<IShellLink> ptr;
		Assert::ExpectException<std::runtime_error>([&] {
			ptr.get();
		});
	}
	
	TEST_METHOD(queryInterface_validPtr) {
		CoPtr<IShellLink> ptr_shell_link(CLSID_ShellLink);
		CoPtr<IUnknown> ptr_unknown = ptr_shell_link.queryInterface<IUnknown>();
		Assert::AreNotEqual(static_cast<void*>(ptr_shell_link.get()), static_cast<void*>(ptr_unknown.get()));
	}
	
	TEST_METHOD(queryInterface_nullFails) {
		CoPtr<IShellLink> ptr;
		Assert::ExpectException<std::runtime_error>([&] {
			ptr.queryInterface<IUnknown>();
		});
	}
	
	TEST_METHOD(outPtr_nullSucceeds) {
		IShellLink* raw_ptr;
		{
			CoPtr<IShellLink> ptr;
			IShellLink** address = ptr.outPtr();
			Assert::IsNotNull(address);
			Assert::IsNull(*address);
			
			Assert::IsTrue(SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(address))));
			raw_ptr = *address;
			
			Assert::AreEqual(raw_ptr, ptr.get());
			raw_ptr->AddRef();
		}
		assertOneReference(raw_ptr);
	}
	
	TEST_METHOD(outPtr_validPtrFails) {
		CoPtr<IShellLink> ptr(CLSID_ShellLink);
		Assert::ExpectException<std::runtime_error>([&] {
			ptr.outPtr();
		});
	}
	
	TEST_METHOD(outPtrVoid_nullSucceeds) {
		IShellItem* raw_ptr;
		{
			CoPtr<IShellItem> ptr;
			void** address = ptr.outPtrVoid();
			Assert::IsNotNull(address);
			Assert::IsNull(*address);
			
			Assert::IsTrue(SUCCEEDED(SHCreateItemFromParsingName(
				_T(""), /* IBindCtx*= */ nullptr, IID_IShellItem, address)));
			raw_ptr = static_cast<IShellItem*>(*address);
			
			Assert::AreEqual(raw_ptr, ptr.get());
			raw_ptr->AddRef();
		}
		assertOneReference(raw_ptr);
	}
	
	TEST_METHOD(outPtrVoid_validPtrFails) {
		CoPtr<IShellLink> ptr(CLSID_ShellLink);
		Assert::ExpectException<std::runtime_error>([&] {
			ptr.outPtrVoid();
		});
	}
	
private:
	
	void assertOneReference(IUnknown* object) {
		Assert::AreEqual(2UL, object->AddRef());
		Assert::AreEqual(1UL, object->Release());
	}
	
	template<class T>
	void assertOneReference(CoPtr<T>& ptr) {
		Assert::AreEqual(2UL, ptr->AddRef());
		Assert::AreEqual(1UL, ptr->Release());
	}
};

TEST_CLASS(CoBufferTest) {
public:
	
	TEST_METHOD(constructorEmpty) {
		CoBuffer<LPSTR> buffer;
		Assert::IsNull(LPSTR(buffer));
	}
	
	TEST_METHOD(constructorTakeOwnership) {
		LPSTR raw_buffer = LPSTR(CoTaskMemAlloc(10));
		StringCchCopyA(raw_buffer, 10, "test");
		
		CoBuffer<LPSTR> buffer(raw_buffer);
		Assert::IsNotNull(LPSTR(buffer));
		Assert::AreSame(*raw_buffer, *LPSTR(buffer));
	}
	
	TEST_METHOD(destructor_freesValid) {
		LPSTR raw_buffer = LPSTR(CoTaskMemAlloc(10));
		
		CoPtr<IMalloc> malloc;
		Assert::IsTrue(SUCCEEDED(CoGetMalloc(1, malloc.outPtr())));
		Assert::AreEqual(1, malloc->DidAlloc(raw_buffer));
		
		{
			CoBuffer<LPSTR> buffer(raw_buffer);
		}
		
		Assert::AreEqual(0, malloc->DidAlloc(raw_buffer));
	}
	
	TEST_METHOD(outPtr_nullSucceeds) {
		LPSTR raw_buffer = LPSTR(CoTaskMemAlloc(10));
		
		CoBuffer<LPSTR> buffer;
		LPSTR *address = buffer.outPtr();
		Assert::IsNotNull(address);
		Assert::IsNull(*address);
		
		*address = raw_buffer;
		Assert::AreEqual(raw_buffer, buffer);
	}
	
	TEST_METHOD(outPtr_validPtrFails) {
		CoBuffer<LPSTR> buffer(LPSTR(CoTaskMemAlloc(10)));
		Assert::ExpectException<std::runtime_error>([&] {
			buffer.outPtr();
		});
	}
};

}  // namespace ComTest
