#pragma once

#include <cctype>
#include <cstdio>
#include <vector>

inline bool ch_iless(char c1, char c2)
{
    return std::tolower(c1) < std::tolower(c2);
}

inline bool str_iless(const std::string& left, const std::string& right) noexcept
{
    return std::lexicographical_compare(left.cbegin(), left.cend(), right.cbegin(), right.cend(), ch_iless);
}

struct case_insensitive_str_less
{
    bool operator()(const std::string& left, const std::string& right) const
    {
        return str_iless(left, right);
    }
};

template <typename char_t>
void StringCleanup(std::basic_string<char_t>& str, const char_t* charsToRemove)
{
    while (true)
    {
        auto pos = str.find_first_of(charsToRemove);
        if (pos == std::basic_string<char_t>::npos)
        {
            break;
        }
        str.erase(pos, 1);
    }
}

int StringCompareCaseInsensitive(PCSTR left, PCSTR right);

inline HRESULT StringConcatenate(_Inout_updates_(cchDest) PSTR dest, size_t cchDest, _In_ PCSTR src)
{
#if defined(__STDC_LIB_EXT1__) || defined(__STDC_SECURE_LIB__)
    return (strcat_s(dest, cchDest, src) == 0) ? S_OK : STRSAFE_E_INSUFFICIENT_BUFFER;
#else
    auto totalLen = strlen(dest) + strlen(src);
    if (totalLen < cchDest)
    {
        strcat(dest, src);
        return S_OK;
    }
    return STRSAFE_E_INSUFFICIENT_BUFFER;
#endif
}

template <typename... Types>
int StringPrintfEx(Types&&... args)
{
#if defined(__STDC_LIB_EXT1__) || defined(__STDC_SECURE_LIB__)
    return sprintf_s(std::forward<Types>(args)...);
#else
    return snprintf(std::forward<Types>(args)...);
#endif
}

template <typename... Types>
HRESULT StringPrintf(PSTR buf, size_t len, PCSTR fmt, Types&&... args)
{
    const int cchWritten = StringPrintfEx(buf, len, fmt, std::forward<Types>(args)...);
    return ((0 < cchWritten) && ((UINT)cchWritten < len)) ? S_OK : STRSAFE_E_INSUFFICIENT_BUFFER;
}

// HRESULT return with optional argument to return number of characters written
template <typename... Types>
HRESULT StringPrintf(PSTR buf, size_t len, _In_opt_ int* pWritten, PCSTR fmt, Types&&... args)
{
    const int cchWritten = StringPrintfEx(buf, len, fmt, std::forward<Types>(args)...);
    if ((0 < cchWritten) && ((UINT)cchWritten < len))
    {
        assign_to_opt_param(pWritten, cchWritten);
        return S_OK;
    }
    assign_to_opt_param(pWritten, 0);
    return STRSAFE_E_INSUFFICIENT_BUFFER;
}

template <typename... Types>
HRESULT StringPrintfV(PSTR buf, size_t cchBuf, _In_opt_ int* pWritten, PCSTR fmt, Types&&... args)
{
    const int cchWritten = vsnprintf(buf, cchBuf, fmt, std::forward<Types>(args)...);
    if ((0 < cchWritten) && ((UINT)cchWritten < cchBuf))
    {
        assign_to_opt_param(pWritten, cchWritten);
        return S_OK;
    }
    assign_to_opt_param(pWritten, 0);
    return STRSAFE_E_INSUFFICIENT_BUFFER;
}

template <typename... Types>
auto StringScanf(Types&&... args)
{
#if defined(__STDC_LIB_EXT1__) || defined(__STDC_SECURE_LIB__)
    return sscanf_s(std::forward<Types>(args)...);
#else
    return sscanf(std::forward<Types>(args)...);
#endif
}

std::vector<std::string> StringPartition(const std::string& input, char separator);

namespace docli
{
namespace string_conversions
{

UINT ToUInt(const std::string& val);

}
}
