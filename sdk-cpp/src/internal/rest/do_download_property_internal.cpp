// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_download_property_internal.h"

#include <cassert>
#include <locale>
#include <string>

#include "do_errors.h"
#include "do_error_helpers.h"

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal() = default;

std::error_code CDownloadPropertyValueInternal::Init(const std::string& val) noexcept
{
    return make_error_code(errc::e_not_impl);
}

std::error_code CDownloadPropertyValueInternal::Init(uint32_t val) noexcept
{
    return make_error_code(errc::e_not_impl);
}

std::error_code CDownloadPropertyValueInternal::Init(uint64_t val) noexcept
{
    return make_error_code(errc::e_not_impl);
}

std::error_code CDownloadPropertyValueInternal::Init(bool val) noexcept
{
    return make_error_code(errc::e_not_impl);
}

CDownloadPropertyValueInternal::~CDownloadPropertyValueInternal()
{
}

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(const CDownloadPropertyValueInternal& rhs)
{
    _var = rhs._var;
}

CDownloadPropertyValueInternal& CDownloadPropertyValueInternal::operator=(CDownloadPropertyValueInternal copy)
{
    swap(*this, copy);
    return *this;
}

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(CDownloadPropertyValueInternal&& rhs) noexcept
{
    _var = rhs._var;
    rhs._var = {};
}

const CDownloadPropertyValueInternal::native_type& CDownloadPropertyValueInternal::native_value() const noexcept
{
    return _var;
}

std::error_code CDownloadPropertyValueInternal::As(bool& val) const noexcept
{
    return make_error_code(errc::e_not_impl);
}

std::error_code CDownloadPropertyValueInternal::As(uint32_t& val) const noexcept
{
    return make_error_code(errc::e_not_impl);
}

std::error_code CDownloadPropertyValueInternal::As(uint64_t& val) const noexcept
{
    return make_error_code(errc::e_not_impl);
}

std::error_code CDownloadPropertyValueInternal::As(std::string& val) const noexcept
{
    return make_error_code(errc::e_not_impl);
}

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
