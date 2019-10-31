#pragma once

// ----------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #define PLAT_WINDOWS                1
#elif defined(__ANDROID__)
    #define PLAT_ANDROID                1
#elif defined(__APPLE__)
    #if defined(TARGET_IPHONE_SIMULATOR)
        #define PLAT_IOS_SIM            1
    #elif defined(TARGET_OS_IPHONE)
        #define PLAT_IOS                1
    #elif defined(TARGET_OS_MAC)
        #define PLAT_MAC                1
    #endif // def TARGET_IPHONE_SIMULATOR
#elif defined(__linux__)
    #define PLAT_LINUX                  1
#else
    #error Unable to detect current platform
#endif // def _WIN32 || def __CYGWIN__

#if PLAT_WINDOWS
    #define inline                  __forceinline
    #define Restrict                __restrict
    #define DllImport               __declspec( dllimport )
    #define DllExport               __declspec( dllexport )
    #define CDecl                   __cdecl
    #define Interrupt()             __debugbreak()
    #define IfWin32(x)              x
    #define IfUnix(x)               
#else
    #define IfWin32(x)              
    #define IfUnix(x)               x
#endif // PLAT_WINDOWS

// ----------------------------------------------------------------------------

#ifdef _DEBUG
    #define IfDebug(x)                  x
    #define IfNotDebug(x)               
#else
    #define IfDebug(x)                  
    #define IfNotDebug(x)               x
#endif // def _DEBUG

#ifdef NDEBUG
    #define IfRelease(x)                x
#else
    #define IfRelease(x)                
#endif // def _NDEBUG

// ----------------------------------------------------------------------------

#define countof(x)                  ( sizeof(x) / sizeof((x)[0]) )
#define argof(x)                    x, countof(x)
#define OffsetOf(s, m)              ( (usize)&(((s*)0)->m) )
#define DebugInterrupt()            IfDebug(Interrupt())
#define IfTrue(x, expr)             if(x) { expr; }
#define IfFalse(x, expr)            if(!(x)) { expr; }
#define Assert(x)                   IfFalse(x, Interrupt())
#define DebugAssert(x)              IfDebug(Assert(x))
#define Check0(x)                   IfFalse(x, DebugInterrupt(); return 0)
#define Check(x, expr)              IfFalse(x, DebugInterrupt(); expr)

#define _CatToken(x, y)             x ## y
#define CatToken(x, y)              _CatToken(x, y)

#define StaticAssert(x)             typedef char CatToken(StaticAssertType_, __COUNTER__) [ (x) ? 1 : -1]

#define Min(a, b)                   ( (a) < (b) ? (a) : (b) )
#define Max(a, b)                   ( (a) > (b) ? (a) : (b) )
#define Clamp(x, lo, hi)            ( Min(hi, Max(lo, x)) )
#define Lerp(a, b, t)               ( (a) + (((b) - (a)) * (t)) )

#define AlignGrow(x, y)             ( ((x) + (y) - 1u) & ~((y) - 1u) )
#define AlignGrowT(T, U)            AlignGrow(sizeof(T), sizeof(U))
#define DWordSizeOf(T)              AlignGrowT(T, u32)
#define QWordSizeOf(T)              AlignGrowT(T, u64)
#define PtrSizeOf(T)                AlignGrowT(T, usize)
#define CacheSizeOf(T)              AlignGrow(sizeof(T), 64u)
#define PageSizeOf(T)               AlignGrow(sizeof(T), 4096u)
#define PIM_MAX_PATH                256

// ----------------------------------------------------------------------------

typedef char* VaList;
#define VaStart(arg)                (reinterpret_cast<VaList>(&(arg)) + PtrSizeOf(arg))
#define VaArg(va, T)                *reinterpret_cast<T*>((va += PtrSizeOf(T)) - PtrSizeOf(T))

// ----------------------------------------------------------------------------

