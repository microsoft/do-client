// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_common.h"
#include "string_ops.h"

int StringCompareCaseInsensitive(PCSTR left, PCSTR right)
{
    while ((*left != '\0') && (*right != '\0'))
    {
        if (std::toupper(*left) < std::toupper(*right)) return -1;
        if (std::toupper(*left) > std::toupper(*right)) return 1;
        ++left;
        ++right;
    }
    if ((*left == '\0') && (*right != '\0')) return -1;
    if ((*left != '\0') && (*right == '\0')) return 1;
    return 0;
}

// Split input string into two substrings separated by the specified separator.
// Empty substrings are not included in the return value.
std::vector<std::string> StringPartition(const std::string& input, char separator)
{
    std::vector<std::string> parts;
    auto pos = input.find(separator);
    if (pos != std::string::npos)
    {
        if (pos > 0)
        {
            parts.emplace_back(input.substr(0, pos));
        }

        if (pos < (input.size() - 1))
        {
            parts.emplace_back(input.substr(pos + 1));
        }
    }
    return parts;
}

namespace docli
{
namespace string_conversions
{

UINT ToUInt(const std::string& val)
{
    UINT ret = 0;

    // Catch std exceptions and convert into DOResultException so that
    // upper layer CATCH_RETURN blocks don't convert it to ERROR_UNHANDLED_EXCEPTION.
    try
    {
        ret = std::stoul(val);
    }
    catch(const std::invalid_argument&)
    {
        THROW_HR(E_INVALIDARG);
    }
    catch (const std::out_of_range&)
    {
        THROW_HR(E_INVALIDARG);
    }
    return ret;
}

} // namespace string_conversions
} // namespace docli
