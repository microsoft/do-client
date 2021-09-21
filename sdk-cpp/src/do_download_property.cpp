
#if defined(DO_INTERFACE_COM)
#include "do_download_property.h"

#include <cassert>
#include <locale>
#include <vector>
#include <string>

#include "do_exceptions.h"

using namespace microsoft::deliveryoptimization;

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
    const UINT result = MultiByteToWideChar(CP_UTF8, 0, str, static_cast<int>(cch), dest.data(), static_cast<int>(dest.size()));
    if (result == 0)
    {
        return E_FAIL;
    }
    wstr = std::wstring(dest.data(), result);
    return S_OK;
}

//TODO(jimson): It's a little awkward for callers to have to init download_property_value since
// the class is so small, so for now only throw if exceptions are enabled
download_property_value::download_property_value(const std::string& val)
{
    V_VT(&_var) = VT_BSTR;

    std::wstring wval;
    const auto res = UTF8toWstr(wval, val.c_str());
#if (!DO_DISABLE_EXCEPTIONS)
    throw_if_fail(res);
#endif

    BSTR bstr = SysAllocString(wval.c_str());
    if (bstr == nullptr)
    {
#if (!DO_DISABLE_EXCEPTIONS)
        throw std::bad_alloc();
#endif
    }
    V_BSTR(&_var) = bstr;
};

download_property_value::download_property_value(UINT val)
{
    V_VT(&_var) = VT_UI4;
    V_UI4(&_var) = val;
};

download_property_value::download_property_value(UINT64 val)
{
    V_VT(&_var) = VT_UI8;
    V_UI8(&_var) = val;
};

download_property_value::download_property_value(bool val)
{
    V_VT(&_var) = VT_BOOL;
    V_BOOL(&_var) = val ? VARIANT_TRUE : VARIANT_FALSE;
};

download_property_value::download_property_value(const status_callback_t& val)
{
    _callback = val;
}

download_property_value::download_property_value(std::vector<unsigned char>& val)
{
    throw E_NOTIMPL;
};

download_property_value::~download_property_value()
{
#ifdef DEBUG
    //TODO(jimson): Variant clear fails with DISP_E_BADVARTYPE when used for callbacks, so the assertion will terminate the application
    //assert(SUCCEEDED(VariantClear(&_var)));
    (void)VariantClear(&_var);
#else
    (void)VariantClear(&_var);
#endif
};

download_property_value::download_property_value(const download_property_value& rhs)
{
    const auto hr = VariantCopy(&_var, &rhs._var);
#if (!DO_DISABLE_EXCEPTIONS)
    microsoft::deliveryoptimization::throw_if_fail(hr);
#endif
    _callback = rhs._callback;
};

download_property_value& download_property_value::operator=(download_property_value copy)
{
    swap(*this, copy);
    return *this;
};

download_property_value::download_property_value(download_property_value&& rhs) noexcept
{
    _var = rhs._var;
    rhs._var = {};
    V_VT(&rhs._var) = VT_EMPTY;
    _callback = std::move(rhs._callback);
};

void download_property_value::as(bool& val) const
{
    throw E_NOTIMPL;
};

void download_property_value::as(UINT& val) const
{
    throw E_NOTIMPL;
};

void download_property_value::as(UINT64& val) const
{
    throw E_NOTIMPL;
};

void download_property_value::as(std::string& val) const
{
    throw E_NOTIMPL;
};

void download_property_value::as(status_callback_t& val) const
{
    val = _callback;
};

void download_property_value::as(std::vector<unsigned char>& val) const
{
    throw E_NOTIMPL;
}

const download_property_value::native_type& download_property_value::native_value() const
{
    return _var;
};

#endif // DO_INTERFACE_COM
