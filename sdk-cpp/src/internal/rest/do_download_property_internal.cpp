

#include "do_download_property_internal.h"

#include <cassert>
#include <locale>
#include <vector>
#include <string>

#include "do_exceptions.h"

using namespace microsoft::deliveryoptimization::details;

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(const std::string& val)
{
    _initCode = static_cast<int32_t>(errc::e_not_impl);
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(uint32_t val)
{
    _initCode = static_cast<int32_t>(errc::e_not_impl);
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(uint64_t val)
{
    _initCode = static_cast<int32_t>(errc::e_not_impl);
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(bool val)
{
    _initCode = static_cast<int32_t>(errc::e_not_impl);
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(std::vector<unsigned char>& val)
{
    _initCode = static_cast<int32_t>(errc::e_not_impl);
};

int32_t CDownloadPropertyValueInternal::init_code() const noexcept
{
    return _initCode;
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
    _callback = std::move(rhs._callback);
};

CDownloadPropertyValueInternal::CDownloadPropertyValueInternal(const download_property_value::status_callback_t& val)
{
    _callback = val;
}

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
    return 0;
};

const CDownloadPropertyValueInternal::native_type& CDownloadPropertyValueInternal::native_value() const noexcept
{
    return _var;
};
