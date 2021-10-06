

#include "do_download_property_internal.h"

#include <cassert>
#include <locale>
#include <vector>
#include <string>

#include "do_errors.h"

using namespace microsoft::deliveryoptimization::details;

int32_t CDownloadPropertyValueInternal::Init(const std::string& val) noexcept
{
    return static_cast<int32_t>(errc::e_not_impl);
};

int32_t CDownloadPropertyValueInternal::Init(uint32_t val) noexcept
{
    return static_cast<int32_t>(errc::e_not_impl);
};

int32_t CDownloadPropertyValueInternal::Init(uint64_t val) noexcept
{
    return static_cast<int32_t>(errc::e_not_impl);
};

int32_t CDownloadPropertyValueInternal::Init(bool val) noexcept
{
    return static_cast<int32_t>(errc::e_not_impl);
};

int32_t CDownloadPropertyValueInternal::Init(std::vector<unsigned char>& val) noexcept
{
    return static_cast<int32_t>(errc::e_not_impl);
};

int32_t CDownloadPropertyValueInternal::Init(const download_property_value::status_callback_t& val) noexcept
{
    return static_cast<int32_t>(errc::e_not_impl);
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

int32_t CDownloadPropertyValueInternal::As(bool& val) const noexcept
{
    return static_cast<int32_t>(errc::e_not_impl);
};

int32_t CDownloadPropertyValueInternal::As(uint32_t& val) const noexcept
{
    return static_cast<int32_t>(errc::e_not_impl);
};

int32_t CDownloadPropertyValueInternal::As(uint64_t& val) const noexcept
{
    return static_cast<int32_t>(errc::e_not_impl);
};

int32_t CDownloadPropertyValueInternal::As(std::string& val) const noexcept
{
    return static_cast<int32_t>(errc::e_not_impl);
};

int32_t CDownloadPropertyValueInternal::As(std::vector<unsigned char>& val) const noexcept
{
    return static_cast<int32_t>(errc::e_not_impl);
}

int32_t CDownloadPropertyValueInternal::As(download_property_value::status_callback_t& val) const noexcept
{
    val = _callback;
    return S_OK;
};

