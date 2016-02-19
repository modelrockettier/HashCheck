
#pragma once
/*
-----------------------------------------------------------------------
            Normalize the build architecture settings
-----------------------------------------------------------------------
*/
#if defined(_DEBUG) || defined(DEBUG) || !defined(NDEBUG)

  #if defined(_WIN64) || defined(WIN64) || defined(_M_X64)

    #undef  _32_BIT
    #undef  _64_BIT
    #undef  _WIN32
    #undef  WIN32
    #undef  _WIN64
    #undef  WIN64
    #undef  _DEBUG
    #undef  DEBUG
    #undef  NDEBUG

    // 64-bit DEBUG

    #define _64_BIT
    #define _WIN32
    #define _WIN64
    #define WIN64
    #define _DEBUG
    #define DEBUG

  #else

    #undef  _32_BIT
    #undef  _64_BIT
    #undef  _WIN32
    #undef  WIN32
    #undef  _WIN64
    #undef  WIN64
    #undef  _DEBUG
    #undef  DEBUG
    #undef  NDEBUG

    // 32-bit DEBUG

    #define _32_BIT
    #define _WIN32
    #define WIN32
    #define _DEBUG
    #define DEBUG

  #endif
#else

  #if defined(_WIN64) || defined(WIN64) || defined(_M_X64)

    #undef  _32_BIT
    #undef  _64_BIT
    #undef  _WIN32
    #undef  WIN32
    #undef  _WIN64
    #undef  WIN64
    #undef  _DEBUG
    #undef  DEBUG
    #undef  NDEBUG

    // 64-bit Release

    #define _64_BIT
    #define _WIN32
    #define _WIN64
    #define WIN64
    #define NDEBUG

  #else

    #undef  _32_BIT
    #undef  _64_BIT
    #undef  _WIN32
    #undef  WIN32
    #undef  _WIN64
    #undef  WIN64
    #undef  _DEBUG
    #undef  DEBUG
    #undef  NDEBUG

    // 32-bit Release

    #define _32_BIT
    #define _WIN32
    #define WIN32
    #define NDEBUG

  #endif
#endif
