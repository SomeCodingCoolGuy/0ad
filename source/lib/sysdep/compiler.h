/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * compiler-specific macros and fixes
 */

#ifndef INCLUDED_COMPILER
#define INCLUDED_COMPILER

#include "lib/sysdep/arch.h"	// ARCH_AMD64
#include "lib/config.h"	// CONFIG_OMIT_FP


// detect compiler and its version (0 if not present, otherwise
// major*100 + minor). note that more than one *_VERSION may be
// non-zero due to interoperability (e.g. ICC with MSC).
// .. VC
#ifdef _MSC_VER
# define MSC_VERSION _MSC_VER
#else
# define MSC_VERSION 0
#endif
// .. ICC (VC-compatible)
#if defined(__INTEL_COMPILER)
# define ICC_VERSION __INTEL_COMPILER
#else
# define ICC_VERSION 0
#endif
// .. LCC (VC-compatible)
#if defined(__LCC__)
# define LCC_VERSION __LCC__
#else
# define LCC_VERSION 0
#endif
// .. GCC
#ifdef __GNUC__
# define GCC_VERSION (__GNUC__*100 + __GNUC_MINOR__)
#else
# define GCC_VERSION 0
#endif


// pass "omit frame pointer" setting on to the compiler
#if MSC_VERSION && !ARCH_AMD64
# if CONFIG_OMIT_FP
#  pragma optimize("y", on)
# else
#  pragma optimize("y", off)
# endif
#elif GCC_VERSION
// TODO
#endif


// are PreCompiled Headers supported?
#if MSC_VERSION
# define HAVE_PCH 1
#elif defined(USING_PCH)
# define HAVE_PCH 1
#else
# define HAVE_PCH 0
#endif


// try to define _W64, if not already done
// (this is useful for catching pointer size bugs)
#ifndef _W64
# if MSC_VERSION
#  define _W64 __w64
# elif GCC_VERSION
#  define _W64 __attribute__((mode (__pointer__)))
# else
#  define _W64
# endif
#endif


// check if compiling in pure C mode (not C++) with support for C99.
// (this is more convenient than testing __STDC_VERSION__ directly)
//
// note: C99 provides several useful but disjunct bits of functionality.
// unfortunately, most C++ compilers do not offer a complete implementation.
// however, many of these features are likely to be added to C++, and/or are
// already available as extensions. what we'll do is add a HAVE_ macro for
// each feature and test those instead. they are set if HAVE_C99, or also if
// the compiler happens to support something compatible.
//
// rationale: lying about __STDC_VERSION__ via Premake so as to enable support
// for some C99 functions doesn't work. Mac OS X headers would then use the
// restrict keyword, which is never supported by g++ (because that might
// end up breaking valid C++98 programs).
#define HAVE_C99 0
#ifdef __STDC_VERSION__
# if __STDC_VERSION__ >= 199901L
#  undef  HAVE_C99
#  define HAVE_C99 1
# endif
#endif


// (at least rudimentary) support for C++0x
#ifndef HAVE_CPP0X
# if defined(__GXX_EXPERIMENTAL_CPP0X__) || MSC_VERSION >= 1600 || ICC_VERSION >= 1200
#  define HAVE_CPP0X 1
# else
#  define HAVE_CPP0X 0
# endif
#endif


// C99-like restrict (non-standard in C++, but widely supported in various forms).
//
// May be used on pointers. May also be used on member functions to indicate
// that 'this' is unaliased (e.g. "void C::m() RESTRICT { ... }").
// Must not be used on references - GCC supports that but VC doesn't.
//
// We call this "RESTRICT" to avoid conflicts with VC's __declspec(restrict),
// and because it's not really the same as C99's restrict.
//
// To be safe and satisfy the compilers' stated requirements: an object accessed
// by a restricted pointer must not be accessed by any other pointer within the
// lifetime of the restricted pointer, if the object is modified.
// To maximise the chance of optimisation, any pointers that could potentially
// alias with the restricted one should be marked as restricted too.
//
// It would probably be a good idea to write test cases for any code that uses
// this in an even very slightly unclear way, in case it causes obscure problems
// in a rare compiler due to differing semantics.
//
// .. GCC
#if GCC_VERSION
# define RESTRICT __restrict__
// .. VC8 provides __restrict
#elif MSC_VERSION >= 1400
# define RESTRICT __restrict
// .. ICC supports the keyword 'restrict' when run with the /Qrestrict option,
//    but it always also supports __restrict__ or __restrict to be compatible
//    with GCC/MSVC, so we'll use the underscored version. One of {GCC,MSC}_VERSION
//    should have been defined in addition to ICC_VERSION, so we should be using
//    one of the above cases (unless it's an old VS7.1-emulating ICC).
#elif ICC_VERSION
# error ICC_VERSION defined without either GCC_VERSION or an adequate MSC_VERSION
// .. unsupported; remove it from code
#else
# define RESTRICT
#endif


// C99-style __func__
// .. newer GCC already have it
#if GCC_VERSION >= 300
	// nothing need be done
// .. old GCC and MSVC have __FUNCTION__
#elif GCC_VERSION >= 200 || MSC_VERSION
# define __func__ __FUNCTION__
// .. unsupported
#else
# define __func__ "(unknown)"
#endif


// tell the compiler that the code at/following this macro invocation is
// unreachable. this can improve optimization and avoid warnings.
//
// this macro should not generate any fallback code; it is merely the
// compiler-specific backend for lib.h's UNREACHABLE.
// #define it to nothing if the compiler doesn't support such a hint.
#define HAVE_ASSUME_UNREACHABLE 1
#if MSC_VERSION && !ICC_VERSION // (ICC ignores this)
# define ASSUME_UNREACHABLE __assume(0)
#elif GCC_VERSION >= 450
# define ASSUME_UNREACHABLE __builtin_unreachable()
#else
# define ASSUME_UNREACHABLE
# undef HAVE_ASSUME_UNREACHABLE
# define HAVE_ASSUME_UNREACHABLE 0
#endif


// extern "C", but does the right thing in pure-C mode
#if defined(__cplusplus)
# define EXTERN_C extern "C"
#else
# define EXTERN_C extern
#endif

#if MSC_VERSION
# define INLINE __forceinline
#else
# define INLINE inline
#endif

#if MSC_VERSION
# define CALL_CONV __cdecl
#else
# define CALL_CONV
#endif

#if MSC_VERSION && !ARCH_AMD64
# define DECORATED_NAME(name) _##name
#else
# define DECORATED_NAME(name) name
#endif

// workaround for preprocessor limitation: macro args aren't expanded
// before being pasted.
#define STRINGIZE2(id) # id
#define STRINGIZE(id) STRINGIZE2(id)

#endif	// #ifndef INCLUDED_COMPILER
