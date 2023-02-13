// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_download_property_internal.h"

#include <vector>

#include "do_error_helpers.h"

namespace msdo = microsoft::deliveryoptimization;

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

static std::error_code UTF8toWstr(const std::string& str, std::wstring& wstr)
{
    wstr.clear();
    size_t cch = str.size();
    if (cch != 0)
    {
        std::vector<wchar_t> dest(cch * 4);
        const int result = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(cch), dest.data(), static_cast<int>(dest.size()));
        if (result == 0)
        {
            return make_error_code(HRESULT_FROM_WIN32(::GetLastError()));
        }
        wstr = std::wstring(dest.data(), static_cast<size_t>(result));
    }
    return DO_OK;
}

static std::error_code WstrToUTF8(const std::wstring& wstr, std::string& str)
{
    str.clear();
    size_t cch = wstr.size();
    if (cch != 0)
    {
        std::vector<char> dest(cch * 4);
        const int result = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(cch), dest.data(), static_cast<int>(dest.size()), 0, 0);
        if (result == 0)
        {
            return make_error_code(HRESULT_FROM_WIN32(::GetLastError()));
        }
        str = std::string(dest.data(), static_cast<size_t>(result));
    }
    return DO_OK;
}

unique_variant::unique_variant()
{
    VariantInit(this);
}

unique_variant::unique_variant(const VARIANT& other) noexcept :
    VARIANT(other)
{
}

unique_variant::unique_variant(unique_variant&& other) noexcept :
    VARIANT(other)
{
    VariantInit(&other);
}

unique_variant& unique_variant::operator=(unique_variant&& other) noexcept
{
    VariantClear(this);
    VARIANT::operator=(other);
    VariantInit(&other);
    return *this;
}

unique_variant::~unique_variant()
{
    VariantClear(this);
}

std::error_code CDownloadPropertyValueInternal::Init(const std::string& val) noexcept
{
    std::wstring wval;
    DO_RETURN_IF_FAILED(UTF8toWstr(val, wval));
    return Init(wval);
}

std::error_code CDownloadPropertyValueInternal::Init(const std::wstring& val) noexcept
{
    BSTR bstr = SysAllocString(val.c_str());
    if (bstr == nullptr)
    {
        return msdo::details::make_error_code(std::errc::not_enough_memory);
    }

    V_VT(&_var) = VT_BSTR;
    V_BSTR(&_var) = bstr;
    return DO_OK;
}

std::error_code CDownloadPropertyValueInternal::Init(uint32_t val) noexcept
{
    V_VT(&_var) = VT_UI4;
    V_UI4(&_var) = val;
    return DO_OK;
}

std::error_code CDownloadPropertyValueInternal::Init(uint64_t val) noexcept
{
    V_VT(&_var) = VT_UI8;
    V_UI8(&_var) = val;
    return DO_OK;
}

std::error_code CDownloadPropertyValueInternal::Init(bool val) noexcept
{
    V_VT(&_var) = VT_BOOL;
    V_BOOL(&_var) = val ? VARIANT_TRUE : VARIANT_FALSE;
    return DO_OK;
}

std::error_code CDownloadPropertyValueInternal::As(bool& val) const noexcept
{
    val = false;
    unique_variant v2;
    RETURN_IF_FAILED(VariantChangeType(&v2, &_var, 0, VT_BOOL));
    val = (V_BOOL(&v2) != VARIANT_FALSE);
    return DO_OK;
}

std::error_code CDownloadPropertyValueInternal::As(uint32_t& val) const noexcept
{
    val = 0;
    unique_variant v2;
    RETURN_IF_FAILED(VariantChangeType(&v2, &_var, 0, VT_UI4));
    val = V_UI4(&v2);
    return DO_OK;
}

std::error_code CDownloadPropertyValueInternal::As(uint64_t& val) const noexcept
{
    val = 0;
    unique_variant v2;
    RETURN_IF_FAILED(VariantChangeType(&v2, &_var, 0, VT_UI8));
    val = V_UI8(&v2);
    return DO_OK;
}

std::error_code CDownloadPropertyValueInternal::As(std::string& val) const noexcept
{
    val.clear();
    std::wstring wstr;
    DO_RETURN_IF_FAILED(As(wstr));
    DO_RETURN_IF_FAILED(WstrToUTF8(wstr, val));
    return DO_OK;
}

std::error_code CDownloadPropertyValueInternal::As(std::wstring& val) const noexcept
{
    val.clear();
    if (V_VT(&_var) == VT_BSTR)
    {
        val = V_BSTR(&_var); // avoid the extra string copy from VariantChangeType
    }
    else
    {
        unique_variant v2;
        RETURN_IF_FAILED(VariantChangeType(&v2, &_var, 0, VT_BSTR));
        val = V_BSTR(&v2);
    }
    return DO_OK;
}

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
