// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_download_property_internal.h"

#include <cassert>
#include <vector>
#include <string>

#include "do_errors.h"
#include "do_error_helpers.h"

namespace msdo = microsoft::deliveryoptimization;

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

static std::error_code UTF8toWstr(const char* str, std::wstring& wstr)
{
    size_t cch = strlen(str);
    if (cch == 0)
    {
        wstr = std::wstring();
    }

    std::vector<wchar_t> dest(cch * 4);
    const int result = MultiByteToWideChar(CP_UTF8, 0, str, static_cast<int>(cch), dest.data(), static_cast<int>(dest.size()));
    if (result == 0)
    {
        return make_error_code(HRESULT_FROM_WIN32(::GetLastError()));
    }
    wstr = std::wstring(dest.data(), static_cast<UINT>(result));
    return DO_OK;
}

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal()
{
    VariantInit(&_var);
}

std::error_code CDownloadPropertyValueInternal::Init(const std::string& val) noexcept
{
    V_VT(&_var) = VT_BSTR;

    std::wstring wval;
    auto hr = UTF8toWstr(val.c_str(), wval);
    DO_RETURN_IF_FAILED(hr);

    BSTR bstr = SysAllocString(wval.c_str());
    if (bstr == nullptr)
    {
        return msdo::details::make_error_code(std::errc::not_enough_memory);
    }
    V_BSTR(&_var) = bstr;

    return DO_OK;
};

std::error_code CDownloadPropertyValueInternal::Init(uint32_t val) noexcept
{
    V_VT(&_var) = VT_UI4;
    V_UI4(&_var) = val;
    return DO_OK;
};

std::error_code CDownloadPropertyValueInternal::Init(uint64_t val) noexcept
{
    V_VT(&_var) = VT_UI8;
    V_UI8(&_var) = val;
    return DO_OK;
};

std::error_code CDownloadPropertyValueInternal::Init(bool val) noexcept
{
    V_VT(&_var) = VT_BOOL;
    V_BOOL(&_var) = val ? VARIANT_TRUE : VARIANT_FALSE;
    return DO_OK;
};

std::error_code CDownloadPropertyValueInternal::Init(std::vector<unsigned char>& val) noexcept
{
    return make_error_code(errc::e_not_impl);
};

std::error_code CDownloadPropertyValueInternal::Init(const download_property_value::status_callback_t& val) noexcept
{
    _callback = val;
    return DO_OK;
}

CDownloadPropertyValueInternal::~CDownloadPropertyValueInternal()
{
#ifdef DEBUG
    assert(SUCCEEDED(VariantClear(&_var)));
#else
    (void)VariantClear(&_var);
#endif
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(const CDownloadPropertyValueInternal& rhs)
{
    HRESULT res = VariantCopy(&_var, &rhs._var);
#if DEBUG
    assert(SUCCEEDED(res));
#endif
    if (FAILED(res))
    {
        std::terminate();
    }
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
    V_VT(&rhs._var) = VT_EMPTY;
    _callback = std::move(rhs._callback);
};

const CDownloadPropertyValueInternal::native_type& CDownloadPropertyValueInternal::native_value() const noexcept
{
    return _var;
};

std::error_code CDownloadPropertyValueInternal::As(bool& val) const noexcept
{
    return make_error_code(errc::e_not_impl);
};

std::error_code CDownloadPropertyValueInternal::As(uint32_t& val) const noexcept
{
    return make_error_code(errc::e_not_impl);
};

std::error_code CDownloadPropertyValueInternal::As(uint64_t& val) const noexcept
{
    return make_error_code(errc::e_not_impl);
};

std::error_code CDownloadPropertyValueInternal::As(std::string& val) const noexcept
{
    return make_error_code(errc::e_not_impl);
};

std::error_code CDownloadPropertyValueInternal::As(std::vector<unsigned char>& val) const noexcept
{
    return make_error_code(errc::e_not_impl);
}

std::error_code CDownloadPropertyValueInternal::As(download_property_value::status_callback_t& val) const noexcept
{
    val = _callback;
    return DO_OK;
};

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
