

#include "do_download_property_internal.h"

#include <cassert>
#include <locale>
#include <vector>
#include <string>

#include "do_exceptions.h"

using namespace microsoft::deliveryoptimization::details;

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(const std::string& val)
{
    throw errc::e_not_impl;
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(uint32_t val)
{
    throw errc::e_not_impl;
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(uint64_t val)
{
    throw errc::e_not_impl;
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(bool val)
{
    throw errc::e_not_impl;
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(std::vector<unsigned char>& val)
{
    throw errc::e_not_impl;
};

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
    _callback = std::move(rhs._callback);
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(const download_property_value::status_callback_t& val)
{
    _callback = val;
}

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

const CDownloadPropertyValueInternal::native_type& CDownloadPropertyValueInternal::native_value() const noexcept
{
    return _var;
};
