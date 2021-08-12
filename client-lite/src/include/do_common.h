#pragma once

// Note: Secure C string functions like wcscat_s, swscanf_s require __STDC_WANT_LIB_EXT1__
// to be defined prior to including stdio.h. However GCC hasn't implemented these functions
// yet so we conditionally compile with __STDC_LIB_EXT1__ currently.
// On the windows side, these are always available and we use __STDC_SECURE_LIB__ to test
// for presence of these functions.
#ifndef __STDC_WANT_LIB_EXT1__
#define __STDC_WANT_LIB_EXT1__  1
#endif

#include <cstddef>  // size_t
#include <cstring>
#include <string>

#if !defined(DEBUG) && !defined(NDEBUG)
#define DEBUG
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(A)    (sizeof(A)/sizeof((A)[0]))
#endif

#ifndef INTERNET_DEFAULT_PORT
#define INTERNET_DEFAULT_PORT   0   // use the protocol-specific default
#endif

#include "sal_undef.h"

// Assign the given value to an optional output parameter.
// Makes code more concise by removing trivial if (outParam) blocks.
template <typename T>
inline void assign_to_opt_param(_Out_opt_ T* outParam, T val)
{
    if (outParam != nullptr)
    {
        *outParam = val;
    }
}

#include "basic_types.h"
#include "error_macros.h" // required by headers below
#include "do_assert.h"
#include "hresult_helpers.h"
#include "do_log.h"
