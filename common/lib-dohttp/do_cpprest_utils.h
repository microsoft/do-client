/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Utilities
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#ifndef _DELIVERY_OPTIMIZATION_DO_CPPREST_UTILS_H
#define _DELIVERY_OPTIMIZATION_DO_CPPREST_UTILS_H

#include <climits>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{
namespace cpprest_utils
{

/*
    Code borrowed from cpprestsdk project: https://github.com/microsoft/cpprestsdk/
    libcurl includes APIs to work with URLs starting from v7.62.0.
    Ubuntu 18.04 ships with libcurl v7.58.0 only so we cannot use the URL API.
 */

#ifndef __cdecl
#define __cdecl
#endif

#ifndef _ASYNCRTIMP
#define _ASYNCRTIMP
#endif

#ifndef CPPREST_NOEXCEPT
#define CPPREST_NOEXCEPT noexcept
#define CPPREST_CONSTEXPR constexpr
#endif // !CPPREST_NOEXCEPT

#ifndef _XPLATSTR
#define _XPLATSTR(x) x
#define ucout std::cout
#define ucin std::cin
#define ucerr std::cerr
#endif

// All strings are narrow for DO
typedef char char_t;
typedef std::string string_t;
typedef std::string utf8string;
typedef std::ostringstream ostringstream_t;
typedef std::ofstream ofstream_t;
typedef std::ostream ostream_t;
typedef std::istream istream_t;
typedef std::ifstream ifstream_t;
typedef std::istringstream istringstream_t;
typedef std::stringstream stringstream_t;

/// <summary>
/// Convert a string to lowercase in place.
/// </summary>
/// <param name="target">The string to convert to lowercase.</param>
_ASYNCRTIMP void __cdecl inplace_tolower(std::string& target) CPPREST_NOEXCEPT;

inline const utf8string& print_utf8string(const utf8string& val) { return val; }
inline const string_t& print_string(const string_t& val) { return val; }

/// <summary>
/// Converts to a UTF-8 string.
/// </summary>
/// <param name="value">A single byte character UTF-8 string.</param>
/// <returns>A single byte character UTF-8 string.</returns>
inline std::string&& to_utf8string(std::string&& value) { return std::move(value); }

/// <summary>
/// Converts to a UTF-8 string.
/// </summary>
/// <param name="value">A single byte character UTF-8 string.</param>
/// <returns>A single byte character UTF-8 string.</returns>
inline const std::string& to_utf8string(const std::string& value) { return value; }

inline string_t&& to_string_t(std::string&& s) { return std::move(s); }
inline const string_t& to_string_t(const std::string& s) { return s; }

template<class T>
inline string_t to_string_t(const T t)
{
    return std::to_string(t);
}

} // namespace cpprest_utils
} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft

#endif // _DELIVERY_OPTIMIZATION_DO_CPPREST_UTILS_H
