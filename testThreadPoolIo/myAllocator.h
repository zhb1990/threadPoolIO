#pragma once
/* The following code example is taken from the book
* "The C++ Standard Library - A Tutorial and Reference"
* by Nicolai M. Josuttis, Addison-Wesley, 1999
*
* (C) Copyright Nicolai M. Josuttis 1999.
* Permission to copy, use, modify, sell and distribute this software
* is granted provided this copyright notice appears in all copies.
* This software is provided "as is" without express or implied
* warranty, and with no claim as to its suitability for any purpose.
*/
#include <limits>
#include <iostream>
#include <jemalloc/jemalloc.h>

template <class T>
class MyAlloc {
public:
	// type definitions
	typedef T        value_type;
	typedef T*       pointer;
	typedef const T* const_pointer;
	typedef T&       reference;
	typedef const T& const_reference;
	typedef std::size_t    size_type;
	typedef std::ptrdiff_t difference_type;

	// rebind allocator to type U
	template <class U>
	struct rebind {
		typedef MyAlloc<U> other;
	};

	// return address of values
	pointer address(reference value) const {
		return &value;
	}
	const_pointer address(const_reference value) const {
		return &value;
	}

	/* constructors and destructor
	* - nothing to do because the allocator has no state
	*/
	MyAlloc() throw() {
	}
	MyAlloc(const MyAlloc&) throw() {
	}
	template <class U>
	MyAlloc(const MyAlloc<U>&) throw() {
	}
	~MyAlloc() throw() {
	}

	// return maximum number of elements that can be allocated
	size_type max_size() const throw() {
		return std::numeric_limits<std::size_t>::max() / sizeof(T);
	}

	// allocate but don't initialize num elements of type T
	pointer allocate(size_type num, const void* = 0) {
		pointer ret = (pointer)(je_malloc(num * sizeof(T)));
		return ret;
	}

	// initialize elements of allocated storage p with value value
	template<class _Objty,
		class... _Types>
		void construct(_Objty *_Ptr, _Types&&... _Args)
	{	// construct _Objty(_Types...) at _Ptr
		::new ((void *)_Ptr) _Objty(_STD forward<_Types>(_Args)...);
	}

	template<class _Uty>
	void destroy(_Uty *_Ptr)
	{	// destroy object at _Ptr
		_Ptr->~_Uty();
	}

	// deallocate storage p of deleted elements
	void deallocate(pointer p, size_type num) {
		je_free((void*)p);
	}
};

// return that all specializations of this allocator are interchangeable
template <class T1, class T2>
bool operator== (const MyAlloc<T1>&,
	const MyAlloc<T2>&) throw() {
	return true;
}
template <class T1, class T2>
bool operator!= (const MyAlloc<T1>&,
	const MyAlloc<T2>&) throw() {
	return false;
}