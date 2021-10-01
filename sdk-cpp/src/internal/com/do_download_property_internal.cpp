

#include "do_download_property_internal.h"

#include <cassert>
#include <vector>
#include <string>

#include "do_exceptions.h"

using namespace microsoft::deliveryoptimization::details;

int32_t UTF8toWstr(std::wstring& wstr, const char* str, size_t cch = 0)
{
    if (cch == 0)
    {
        cch = strlen(str);
    }

    if (cch == 0)
    {
        wstr = std::wstring();
    }

    std::vector<wchar_t> dest(cch * 4);
    const uint32_t result = MultiByteToWideChar(CP_UTF8, 0, str, static_cast<int>(cch), dest.data(), static_cast<int>(dest.size()));
    if (result == 0)
    {
        return E_FAIL;
    }
    wstr = std::wstring(dest.data(), result);
    return S_OK;
}

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(const std::string& val)
{
    V_VT(&_var) = VT_BSTR;

    std::wstring wval;
    const auto hr = UTF8toWstr(wval, val.c_str());
#if (DO_ENABLE_EXCEPTIONS)
    throw_if_fail(hr);
#endif
    _initCode = hr;

    BSTR bstr = SysAllocString(wval.c_str());
    if (bstr == nullptr)
    {
 #if (DO_ENABLE_EXCEPTIONS)
        throw_if_fail(DO_ERROR_FROM_STD_ERROR(std::errc::not_enough_memory)); // empty bstring may not always be out of memory error, but should be most of the time
#endif
        _initCode = DO_ERROR_FROM_STD_ERROR(std::errc::not_enough_memory);
    }
    V_BSTR(&_var) = bstr;
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(uint32_t val)
{
    V_VT(&_var) = VT_UI4;
    V_UI4(&_var) = val;
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(uint64_t val)
{
    V_VT(&_var) = VT_UI8;
    V_UI8(&_var) = val;
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(bool val)
{
    V_VT(&_var) = VT_BOOL;
    V_BOOL(&_var) = val ? VARIANT_TRUE : VARIANT_FALSE;
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(std::vector<unsigned char>& val)
{
    throw errc::e_not_impl;
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(const download_property_value::status_callback_t& val)
{
    V_VT(&_var) = VT_EMPTY;
    _callback = val;
}

int32_t CDownloadPropertyValueInternal::init_code() const noexcept
{
    return _initCode;
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
    const auto hr = VariantCopy(&_var, &rhs._var);
#if (DO_ENABLE_EXCEPTIONS)
    microsoft::deliveryoptimization::throw_if_fail(hr);
#endif
    _initCode = hr;
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

int32_t CDownloadPropertyValueInternal::as(bool& val) const noexcept
{
    return static_cast<int32_t>(errc::e_not_impl);
};

int32_t CDownloadPropertyValueInternal::as(uint32_t& val) const noexcept
{
    return static_cast<int32_t>(errc::e_not_impl);
};

int32_t CDownloadPropertyValueInternal::as(uint64_t& val) const noexcept
{
    return static_cast<int32_t>(errc::e_not_impl);
};

int32_t CDownloadPropertyValueInternal::as(std::string& val) const noexcept
{
    return static_cast<int32_t>(errc::e_not_impl);
};

int32_t CDownloadPropertyValueInternal::as(std::vector<unsigned char>& val) const noexcept
{
    return static_cast<int32_t>(errc::e_not_impl);
}

int32_t CDownloadPropertyValueInternal::as(download_property_value::status_callback_t& val) const noexcept
{
    val = _callback;
    return S_OK;
};

