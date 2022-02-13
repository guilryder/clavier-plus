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


#pragma once


// Smart pointer to a COM object. Lightweight equivalent of CComPtr.
template<class T>
class CoPtr {
public:
	
	CoPtr() : m_ptr(nullptr) {}
	
	// Wrapper for CoCreateInstance().
	explicit CoPtr(REFCLSID rclsid) {
		if (FAILED(CoCreateInstance(rclsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_ptr)))) {
			m_ptr = nullptr;
		}
	}
	
	CoPtr(const CoPtr<T>& other) = delete;
	
	CoPtr(CoPtr<T>&& other) : m_ptr(other.m_ptr) {
		other.m_ptr = nullptr;
	}
	
	~CoPtr() {
		if (m_ptr) {
			m_ptr->Release();
		}
	}
	
	CoPtr<T>& operator =(CoPtr<T>&& other) {
		if (m_ptr != other.m_ptr) {
			this->~CoPtr();
			m_ptr = other.m_ptr;
			other.m_ptr = nullptr;
		}
		return *this;
	}
	
	T* operator ->() {
		assert(m_ptr != nullptr);
		return m_ptr;
	}
	
	operator bool() const {
		return m_ptr != nullptr;
	}
	
	// const_cast to allow calling T methods declared as non-const.
	T* get() const {
		assert(m_ptr != nullptr);
		return const_cast<T*>(m_ptr);
	}
	
	// Wrapper for QueryInterface().
	template<class U>
	CoPtr<U> queryInterface() {
		U* queried = nullptr;
		(*this)->QueryInterface(IID_PPV_ARGS(&queried));
		return CoPtr<U>(queried);
	}
	
	// Returns a pointer to this pointer suitable for IID_PPV_ARGS.
	// Available only if the pointer is null.
	T** operator &() {
		assert(m_ptr == nullptr);
		return &m_ptr;
	}
	
private:
	template<class U> friend class CoPtr;
	
	explicit CoPtr(T* ptr) : m_ptr(ptr) {}
	
	T* m_ptr;
};


// Smart pointer to a buffer to free with CoTaskMemFree().
// Takes the pointer type as argument instead of the type to support __unaligned.
template<class P>
class CoBuffer {
public:
	
	CoBuffer() : m_ptr(nullptr) {}
	
	explicit CoBuffer(P ptr) : m_ptr(ptr) {}
	
	~CoBuffer() {
		CoTaskMemFree(m_ptr);
	}
	
	operator P() { return m_ptr; }
	operator const P() const { return m_ptr; }
	
	// Returns a pointer to this pointer suitable for passing to an allocation function.
	// Available only if the pointer is null.
	P* operator &() {
		assert(m_ptr == nullptr);
		return &m_ptr;
	}
	
private:
	P m_ptr;
};
