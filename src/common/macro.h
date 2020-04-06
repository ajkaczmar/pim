#pragma once

#ifdef __cplusplus
    #define PIM_C_BEGIN extern "C" {
    #define PIM_C_END };
#else
    #define PIM_C_BEGIN 
    #define PIM_C_END 
#endif // __cplusplus

// ----------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #define PLAT_WINDOWS            1
#elif defined(__ANDROID__)
    #define PLAT_ANDROID            1
#elif defined(__APPLE__)
    #if defined(TARGET_IPHONE_SIMULATOR)
        #define PLAT_IOS_SIM        1
    #elif defined(TARGET_OS_IPHONE)
        #define PLAT_IOS            1
    #elif defined(TARGET_OS_MAC)
        #define PLAT_MAC            1
    #endif // def TARGET_IPHONE_SIMULATOR
#elif defined(__linux__)
    #define PLAT_LINUX              1
#else
    #error Unable to detect current platform
#endif // def _WIN32 || def __CYGWIN__

#if defined(__arm__) || defined(__arm64__)
    #define PLAT_CPU_ARM            1
    #define PLAT_CPU_X86            0
#else
    #define PLAT_CPU_ARM            0
    #define PLAT_CPU_X86            1
#endif 

// ----------------------------------------------------------------------------

#if PLAT_WINDOWS
    #define IF_WIN(x)               x
    #define IF_UNIX(x)              

    #define INTERRUPT()             __debugbreak()
    #define pim_thread_local        __declspec(thread)

    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#else
    #define IF_WIN(x)               
    #define IF_UNIX(x)              x

    #define INTERRUPT()             raise(SIGTRAP)
    #define pim_thread_local        _Thread_local
#endif // PLAT_WINDOWS

#define PIM_EXPORT                  IF_WIN(__declspec(dllexport))
#define PIM_IMPORT                  IF_WIN(__declspec(dllimport))
#define PIM_CDECL                   IF_WIN(__cdecl)
#define VEC_CALL                    IF_WIN(__vectorcall)
#define pim_inline                  IF_WIN(__forceinline)
#define pim_noalias                 IF_WIN(__restrict)
#define pim_alignas(x)              IF_WIN(__declspec(align(x)))

// ----------------------------------------------------------------------------

#ifdef _DEBUG
    #define IF_DEBUG(x)             x
    #define IFN_DEBUG(x)            (void)0
#else
    #define IF_DEBUG(x)             (void)0
    #define IFN_DEBUG(x)            x
#endif // def _DEBUG

// ----------------------------------------------------------------------------

#define NELEM(x)                    ( sizeof(x) / sizeof((x)[0]) )
#define ARGS(x)                     x, NELEM(x)
#define DBG_INT()                   IF_DEBUG(INTERRUPT())
#define IF_TRUE(x, expr)            do { if(x) { expr; } } while(0)
#define IF_FALSE(x, expr)           do { if(!(x)) { expr; } } while(0)
#define ASSERT(x)                   IF_DEBUG(IF_FALSE(x, INTERRUPT()))
#define CHECK(x)                    IF_FALSE(x, DBG_INT(); err = EFail; return retval)
#define CHECKERR()                  IF_FALSE(err == ESuccess, DBG_INT(); return retval)

#define _CAT_TOK(x, y)              x ## y
#define CAT_TOK(x, y)               _CAT_TOK(x, y)

#define SASSERT(x)                  typedef char CAT_TOK(StaticAssert_, __COUNTER__) [ (x) ? 1 : -1]

#define CONV_ASSERT(O, I) \
    SASSERT(sizeof(O) >= sizeof(I)); \
    SASSERT(_Alignof(O) >= _Alignof(I));

// ----------------------------------------------------------------------------

// Maximum path length, including null terminator
#define PIM_PATH                    256

// ----------------------------------------------------------------------------

PIM_C_BEGIN

typedef signed char             i8;
typedef signed short            i16;
typedef signed int              i32;
typedef signed long long        i64;
typedef i64                     isize;
typedef unsigned char           u8;
typedef unsigned short          u16;
typedef unsigned int            u32;
typedef unsigned long long      u64;
typedef u64                     usize;


#if !defined(NULL)
    #define NULL                0
#endif

#ifndef _STDBOOL
#define _STDBOOL
#define __bool_true_false_are_defined 1
#ifndef __cplusplus
    #define bool                _Bool
    #define false               0
    #define true                1
#endif // __cplusplus
#endif // _STDBOOL

typedef enum
{
    EAlloc_Init = 0,
    EAlloc_Perm,
    EAlloc_Temp,
    EAlloc_TLS,
    EAlloc_Count
} EAlloc;

PIM_C_END

// ----------------------------------------------------------------------------
