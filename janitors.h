/* 
 * Copyright (C) 2013 Chris McClelland
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */
#ifndef JANITORS_H
#define JANITORS_H

#include <libfpgalink.h>

// Utility classes/templates for ensuring that resources are not leaked if an
// exception occurs. The janitor is stack-constructed with a pointer to the
// resource, and when the janitor goes out of scope its destructor frees the
// owned resource.
// 
template<typename T> class Janitor {
	const T *const m_ptr;
	Janitor<T> &operator=(const Janitor<T> &other);
	Janitor(const Janitor<T> &other);
public:
	explicit Janitor(const T *ptr) : m_ptr(ptr) { }
	~Janitor() { delete m_ptr; }
};

template<typename T> class ArrayJanitor {
	const T *const m_ptr;
	ArrayJanitor(const Janitor<T> &other);
	ArrayJanitor<T> &operator=(const ArrayJanitor<T> &other);
public:
	explicit ArrayJanitor(const T *ptr) : m_ptr(ptr) { }
	~ArrayJanitor() { delete []m_ptr; }
};

class LoadFileJanitor {
	uint8 *const m_ptr;
	LoadFileJanitor &operator=(const LoadFileJanitor &other);
	LoadFileJanitor(const LoadFileJanitor &other);
public:
	explicit LoadFileJanitor(uint8 *ptr) : m_ptr(ptr) { }
	~LoadFileJanitor() { flFreeFile(m_ptr); }
};

class FLContextJanitor {
	FLContext *const m_ptr;
	FLContextJanitor &operator=(const FLContextJanitor &other);
	FLContextJanitor(const FLContextJanitor &other);
public:
	explicit FLContextJanitor(FLContext *ptr) : m_ptr(ptr) { }
	~FLContextJanitor() { flClose(m_ptr); }
};

#endif
