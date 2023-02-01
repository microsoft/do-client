// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_PROPERTY_INTERNAL_H
#define _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_PROPERTY_INTERNAL_H

#if defined(DO_INTERFACE_COM)
#include <OAIdl.h>
#else
#include <boost/variant.hpp>
#endif

#include <string>
#include <vector>

#include "do_download_property.h"
#include "do_errors.h"

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

#if defined(DO_INTERFACE_COM)
struct unique_variant : VARIANT
{
    unique_variant();
    explicit unique_variant(const VARIANT& other) noexcept; // takes ownership via shallow copy
    unique_variant(const unique_variant& other); // true copy
    unique_variant(unique_variant&& other) noexcept;
    unique_variant& operator=(unique_variant&& other);
    ~unique_variant();

    unique_variant& operator=(const unique_variant&) = delete;
    unique_variant& operator=(const VARIANT&) = delete;
};
#endif

class CDownloadPropertyValueInternal
{
public:
#if defined(DO_INTERFACE_COM)
    using native_type = unique_variant;
#else
    using native_type = boost::variant<std::string, uint32_t, uint64_t, bool>;
#endif
    CDownloadPropertyValueInternal();

    std::error_code Init(const std::string& val) noexcept;
    std::error_code Init(uint32_t val) noexcept;
    std::error_code Init(uint64_t val) noexcept;
    std::error_code Init(bool val) noexcept;

    ~CDownloadPropertyValueInternal();

    CDownloadPropertyValueInternal(const CDownloadPropertyValueInternal& rhs);
    CDownloadPropertyValueInternal& operator=(CDownloadPropertyValueInternal copy);
    CDownloadPropertyValueInternal(CDownloadPropertyValueInternal&& rhs) noexcept;

    friend void swap(CDownloadPropertyValueInternal& first, CDownloadPropertyValueInternal& second) noexcept
    {
        std::swap(first._var, second._var);
    }

    std::error_code As(bool& val) const noexcept;
    std::error_code As(uint32_t& val) const noexcept;
    std::error_code As(uint64_t& val) const noexcept;
    std::error_code As(std::string& val) const noexcept;

#if defined(DO_INTERFACE_COM)
    std::error_code Init(const std::wstring& val) noexcept;
    std::error_code As(std::wstring& val) const noexcept;
#endif

    const native_type& native_value() const noexcept;

private:
    native_type _var;
};

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft

#endif // _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_PROPERTY_H
