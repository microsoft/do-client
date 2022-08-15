// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_download_property_internal.h"

#include <cassert>
#include <locale>
#include <vector>
#include <string>

#include "do_errors.h"
#include "do_error_helpers.h"

namespace msdo = microsoft::deliveryoptimization;
using namespace microsoft::deliveryoptimization::details;

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal() = default;

std::error_code CDownloadPropertyValueInternal::Init(const std::string& val) noexcept
{
    return msdo::make_error_code(errc::e_not_impl);
};

std::error_code CDownloadPropertyValueInternal::Init(uint32_t val) noexcept
{
    return msdo::make_error_code(errc::e_not_impl);
};

std::error_code CDownloadPropertyValueInternal::Init(uint64_t val) noexcept
{
    return msdo::make_error_code(errc::e_not_impl);
};

std::error_code CDownloadPropertyValueInternal::Init(bool val) noexcept
{
    return msdo::make_error_code(errc::e_not_impl);
};

std::error_code CDownloadPropertyValueInternal::Init(std::vector<unsigned char>& val) noexcept
{
    return msdo::make_error_code(errc::e_not_impl);
};

std::error_code CDownloadPropertyValueInternal::Init(const download_property_value::status_callback_t& val) noexcept
{
    return msdo::make_error_code(errc::e_not_impl);
}

CDownloadPropertyValueInternal::~CDownloadPropertyValueInternal()
{
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(const CDownloadPropertyValueInternal& rhs)
{
    _var = rhs._var;
    _callback = rhs._callback;
};

CDownloadPropertyValueInternal& CDownloadPropertyValueInternal::operator=(CDownloadPropertyValueInternal copy)
{
    swap(*this, copy);
    return *this;
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(CDownloadPropertyValueInternal&& rhs) noexcept
{
    _var = rhs._var;
    rhs._var = {};
    _callback = std::move(rhs._callback);
};

const CDownloadPropertyValueInternal::native_type& CDownloadPropertyValueInternal::native_value() const noexcept
{
    return _var;
};

std::error_code CDownloadPropertyValueInternal::As(bool& val) const noexcept
{
    return msdo::make_error_code(errc::e_not_impl);
};

std::error_code CDownloadPropertyValueInternal::As(uint32_t& val) const noexcept
{
    return msdo::make_error_code(errc::e_not_impl);
};

std::error_code CDownloadPropertyValueInternal::As(uint64_t& val) const noexcept
{
    return msdo::make_error_code(errc::e_not_impl);
};

std::error_code CDownloadPropertyValueInternal::As(std::string& val) const noexcept
{
    return msdo::make_error_code(errc::e_not_impl);
};

std::error_code CDownloadPropertyValueInternal::As(std::vector<unsigned char>& val) const noexcept
{
    return msdo::make_error_code(errc::e_not_impl);
}

std::error_code CDownloadPropertyValueInternal::As(download_property_value::status_callback_t& val) const noexcept
{
    val = _callback;
    return DO_OK;
};

