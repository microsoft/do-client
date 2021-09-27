// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_string_util.h"

#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/case_conv.hpp>

namespace strutil
{

std::string HexEncode(const unsigned char* src, size_t len)
{
    std::string strResult;
    boost::algorithm::hex(src, src + len, std::back_inserter(strResult));
    boost::algorithm::to_lower(strResult);
    return strResult;
}

} // namespace strutil
