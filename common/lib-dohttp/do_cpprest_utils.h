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

#define __cdecl
#define _ASYNCRTIMP

#ifndef CPPREST_NOEXCEPT
#define CPPREST_NOEXCEPT noexcept
#define CPPREST_CONSTEXPR constexpr
#endif // !CPPREST_NOEXCEPT

#ifndef _XPLATSTR
// All strings are narrow for DO
typedef char char_t;
typedef std::string string_t;
typedef std::string utf8string;
#define _XPLATSTR(x) x
typedef std::ostringstream ostringstream_t;
typedef std::ofstream ofstream_t;
typedef std::ostream ostream_t;
typedef std::istream istream_t;
typedef std::ifstream ifstream_t;
typedef std::istringstream istringstream_t;
typedef std::stringstream stringstream_t;
#define ucout std::cout
#define ucin std::cin
#define ucerr std::cerr
#endif // !_XPLATSTR

/// <summary>
/// Convert a string to lowercase in place.
/// </summary>
/// <param name="target">The string to convert to lowercase.</param>
_ASYNCRTIMP void __cdecl inplace_tolower(std::string& target) CPPREST_NOEXCEPT;

/// <summary>
/// Our own implementation of alpha numeric instead of std::isalnum to avoid
/// taking global lock for performance reasons.
/// </summary>
inline bool __cdecl is_alnum(const unsigned char uch) CPPREST_NOEXCEPT
{ // test if uch is an alnum character
    // special casing char to avoid branches
    // clang-format off
    static CPPREST_CONSTEXPR bool is_alnum_table[UCHAR_MAX + 1] = {
        /*        X0 X1 X2 X3 X4 X5 X6 X7 X8 X9 XA XB XC XD XE XF */
        /* 0X */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /* 1X */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /* 2X */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /* 3X */   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, /* 0-9 */
        /* 4X */   0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* A-Z */
        /* 5X */   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
        /* 6X */   0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* a-z */
        /* 7X */   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0
        /* non-ASCII values initialized to 0 */
    };
    // clang-format on
    return (is_alnum_table[uch]);
}

/// <summary>
/// Our own implementation of alpha numeric instead of std::isalnum to avoid
/// taking global lock for performance reasons.
/// </summary>
inline bool __cdecl is_alnum(const char ch) CPPREST_NOEXCEPT { return (is_alnum(static_cast<unsigned char>(ch))); }

inline const utf8string& print_utf8string(const utf8string& val) { return val; }
inline const string_t& print_string(const string_t& val) { return val; }

template<typename Target>
Target scan_string(const string_t& str)
{
    Target t;
    istringstream_t iss(str);
    iss.imbue(std::locale::classic());
    iss >> t;
    if (iss.bad())
    {
        throw std::bad_cast();
    }
    return t;
}

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
