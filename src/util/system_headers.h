/* system_headers.h -- include system headers

   This file is part of the UPX executable compressor.

   Copyright (C) 1996-2025 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   UPX and the UCL library are free software; you can redistribute them
   and/or modify them under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
 */

#pragma once

#include "system_defs.h"
#include "system_features.h"

#if !(__cplusplus + 0 >= 201703L)
#error "FATAL ERROR: C++17 is required"
#endif

// sanity checks
#if defined(_ILP32) || defined(__ILP32) || defined(__ILP32__)
static_assert(sizeof(int) == 4);
static_assert(sizeof(long) == 4);
static_assert(sizeof(void *) == 4);
#endif
#if defined(_LP64) || defined(__LP64) || defined(__LP64__)
static_assert(sizeof(int) == 4);
static_assert(sizeof(long) == 8);
#if defined(__CHERI__) && defined(__CHERI_PURE_CAPABILITY__) && (__SIZEOF_POINTER__ == 16)
// CHERI BUG/FEATURE: obviously CHERI should *NOT* pre-define __LP64__ on P128, but
//   maybe they do this for porting/compatibility reasons...
static_assert(sizeof(void *) == 16);
#else
static_assert(sizeof(void *) == 8);
#endif
#endif
#if defined(_WIN32)
static_assert(sizeof(int) == 4);
static_assert(sizeof(long) == 4);
#if !defined(_WIN64)
static_assert(sizeof(void *) == 4);
#endif
#endif
#if defined(_WIN64)
static_assert(sizeof(int) == 4);
static_assert(sizeof(long) == 4);
static_assert(sizeof(void *) == 8);
#endif
#if defined(__CYGWIN__)
static_assert(sizeof(int) == 4);
static_assert(sizeof(void *) == sizeof(long));
#endif

// ACC and C system headers
#ifndef ACC_CFG_USE_NEW_STYLE_CASTS
#define ACC_CFG_USE_NEW_STYLE_CASTS 1
#endif
#define ACC_CFG_PREFER_TYPEOF_ACC_INT32E_T ACC_TYPEOF_INT
#define ACC_CFG_PREFER_TYPEOF_ACC_INT64E_T ACC_TYPEOF_LONG_LONG
#include "miniacc.h"

// disable some pedantic warnings
#if (ACC_CC_MSC)
#pragma warning(disable : 4127) // W4: conditional expression is constant
#pragma warning(disable : 4244) // W3: conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable : 4267) // W3: conversion from 'size_t' to 'type', possible loss of data
#pragma warning(disable : 4820) // W4: padding added after data member
#if _MSC_VER >= 1800
#pragma warning(disable : 4464) // W4: relative include path contains '..'
#endif
#endif

#undef snprintf
#undef vsnprintf
#define HAVE_STDINT_H       1
#define ACC_WANT_ACC_INCD_H 1
#define ACC_WANT_ACC_INCE_H 1
#define ACC_WANT_ACC_LIB_H  1
#define ACC_WANT_ACC_CXX_H  1
#include "miniacc.h"
#if (ACC_CC_MSC)
#include <intrin.h>
#endif

// C++ freestanding headers
#if __cplusplus >= 202002L
#include <version>
#include <bit>
#endif
#include <cstddef>
#include <cstdint>
#include <exception>
#include <new>
#include <type_traits>
#include <utility>
// C++ system headers
#include <algorithm>
#include <memory> // std::unique_ptr
// C++ multithreading (UPX currently does not use multithreading)
#if __STDC_NO_ATOMICS__
#undef WITH_THREADS
#endif
#if WITH_THREADS
#include <atomic>
#include <mutex>
#endif

// sanitizers: ASAN, MSAN, UBSAN
#if !defined(__SANITIZE_ADDRESS__) && defined(__has_feature)
#if __has_feature(address_sanitizer) // ASAN
#define __SANITIZE_ADDRESS__ 1
#endif
#endif
#if !defined(__SANITIZE_MEMORY__) && defined(__has_feature)
#if __has_feature(memory_sanitizer) // MSAN
#define __SANITIZE_MEMORY__ 1
#endif
#endif
#if !defined(__SANITIZE_UNDEFINED_BEHAVIOR__) && defined(__has_feature)
#if __has_feature(undefined_behavior_sanitizer) // UBSAN
#define __SANITIZE_UNDEFINED_BEHAVIOR__ 1
#endif
#endif

// UPX vendor git submodule headers
#include <doctest/doctest/parts/doctest_fwd.h>
#ifndef WITH_VALGRIND
#define WITH_VALGRIND 1
#endif
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_MEMORY__) || defined(_WIN32) ||            \
    !defined(__GNUC__)
#undef WITH_VALGRIND
#endif
#if defined(__CHERI__) && defined(__CHERI_PURE_CAPABILITY__)
#undef WITH_VALGRIND
#endif
#if WITH_VALGRIND
#include <valgrind/include/valgrind/memcheck.h>
#endif

// IMPORTANT: unconditionally enable assertions
#undef NDEBUG
#include <assert.h>

#if (ACC_OS_CYGWIN || ACC_OS_DOS16 || ACC_OS_DOS32 || ACC_OS_EMX || ACC_OS_OS2 || ACC_OS_OS216 ||  \
     ACC_OS_WIN16 || ACC_OS_WIN32 || ACC_OS_WIN64)
#if defined(INVALID_HANDLE_VALUE) || defined(MAKEWORD) || defined(RT_CURSOR)
#error "something pulled in <windows.h>"
#endif
#endif

#ifdef WANT_WINDOWS_LEAN_H
#if defined(_WIN32) || defined(__CYGWIN__)
#include "windows_lean.h"
#endif
#endif

/* vim:set ts=4 sw=4 et: */
