

#include "do_download_property_internal.h"

#include <cassert>
#include <vector>
#include <string>

#include "do_exceptions.h"

using namespace microsoft::deliveryoptimization::details;

std::wstring UTF8toWstr(const char* str, size_t cch = 0)
{
    if (cch == 0)
    {
        cch = strlen(str);
    }

    if (cch == 0)
    {
        return std::wstring();
    }

    std::vector<wchar_t> dest(cch * 4);
    const UINT result = MultiByteToWideChar(CP_UTF8, 0, str, static_cast<int>(cch), dest.data(), static_cast<int>(dest.size()));
    if (result == 0)
    {
        throw std::exception();
    }
    return std::wstring(dest.data(), result);
}

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(const std::string& val)
{
    V_VT(&_var) = VT_BSTR;

    std::wstring wval = UTF8toWstr(val.c_str());

    BSTR bstr = SysAllocString(wval.c_str());
    if (bstr == nullptr)
    {
        throw std::bad_alloc();
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
    _callback = val;
}

CDownloadPropertyValueInternal::~CDownloadPropertyValueInternal()
{
#ifdef DEBUG
    if (!_callback)
    {
        assert(SUCCEEDED(VariantClear(&_var)));
    }
#else
    (void)VariantClear(&_var);
#endif
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(const CDownloadPropertyValueInternal& rhs)
{
    const auto res = VariantCopy(&_var, &rhs._var);
#if (!DO_DISABLE_EXCEPTIONS)
    microsoft::deliveryoptimization::throw_if_fail(res);
#endif
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

void CDownloadPropertyValueInternal::as(bool& val) const
{
    throw errc::e_not_impl;
};

void CDownloadPropertyValueInternal::as(uint32_t& val) const
{
    throw errc::e_not_impl;
};

void CDownloadPropertyValueInternal::as(uint64_t& val) const
{
    throw errc::e_not_impl;
};

void CDownloadPropertyValueInternal::as(std::string& val) const
{
    throw errc::e_not_impl;
};

void CDownloadPropertyValueInternal::as(std::vector<unsigned char>& val) const
{
    throw errc::e_not_impl;
}

void CDownloadPropertyValueInternal::as(download_property_value::status_callback_t& val) const noexcept
{
    val = _callback;
};

