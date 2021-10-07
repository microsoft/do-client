// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_PROPERTY_INTERNAL_H
#define _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_PROPERTY_INTERNAL_H

#if defined(DO_INTERFACE_COM)
#include <OAIdl.h>
#else
#include <boost/variant.hpp>
#endif

#include <functional>
#include <string>
#include <vector>

#include "do_download_property.h"

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{
class CDownloadPropertyValueInternal
{
public:
#if defined(DO_INTERFACE_COM)
    using native_type = VARIANT;
#else
    using native_type = boost::variant<std::string, uint32_t, uint64_t, bool, std::vector<unsigned char>>;
#endif
    CDownloadPropertyValueInternal() = default;

    int32_t Init(const std::string& val) noexcept;
    int32_t Init(uint32_t val) noexcept;
    int32_t Init(uint64_t val) noexcept;
    int32_t Init(bool val) noexcept;
    int32_t Init(std::vector<unsigned char>& val) noexcept;
    int32_t Init(const download_property_value::status_callback_t& val) noexcept;

    ~CDownloadPropertyValueInternal();

    CDownloadPropertyValueInternal(const CDownloadPropertyValueInternal& rhs);
    CDownloadPropertyValueInternal& operator=(CDownloadPropertyValueInternal copy);
    CDownloadPropertyValueInternal(CDownloadPropertyValueInternal&& rhs) noexcept;

    friend void swap(CDownloadPropertyValueInternal& first, CDownloadPropertyValueInternal& second) noexcept
    {
        std::swap(first._var, second._var);
        std::swap(first._callback, second._callback);
    }

    int32_t As(bool& val) const noexcept;
    int32_t As(uint32_t& val) const noexcept;
    int32_t As(uint64_t& val) const noexcept;
    int32_t As(std::string& val) const noexcept;
    int32_t As(download_property_value::status_callback_t& val) const noexcept;
    int32_t As(std::vector<unsigned char>& val) const noexcept;

    const native_type& native_value() const noexcept;

private:
    native_type _var;
    download_property_value::status_callback_t _callback;
};
} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft

#endif // _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_PROPERTY_H
