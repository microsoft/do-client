#pragma once

#if defined(__x86_64__) || defined(_M_X64) || defined(_M_ARM64) || defined(__aarch64__)
#define DO_ENV_64BIT
#endif

using  INT8  = signed char;
using UINT8  = unsigned char;
using  INT16 = signed short;
using UINT16 = unsigned short;
using  INT32 = signed int;
using UINT32 = unsigned int;
using  INT64 = long long signed int;
using UINT64 = long long unsigned int;

using BYTE = UINT8;

using INT = INT32;
using UINT = UINT32;

// We define HRESULT to be a signed 32bit integer to match Windows.
// Note: Can't use long because hexadecimal literals are forced to be unsigned on GCC, per the standard.
// That is, (long)0x80070490L != -2147023728 and ((long)0x80070490L < 0) evaluates to false.
using HRESULT = INT32;

#ifdef DO_ENV_64BIT
typedef INT64   INT_PTR;
typedef UINT64  UINT_PTR;
#else
typedef int             INT_PTR;
typedef unsigned int    UINT_PTR;
#endif

typedef int             BOOL;

typedef char            CHAR, *PSTR;
typedef const char      *PCSTR;

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    1
#endif

#ifndef MAXUINT32
#define MAXUINT32   ((UINT32)~((UINT32)0))
#define MAXUINT64   ((UINT64)~((UINT64)0))
#define MAXUINT     ((UINT)~((UINT)0))
#endif
