// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_PROPERTY_H
#define _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_PROPERTY_H

#include <memory>
#include <string>

#include "do_errors.h"

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{
class CDownloadImpl;
class CDownloadPropertyValueInternal;
}

enum class download_property
{
    id = 0,                         // string (readonly)
    uri,                            // string
    catalog_id,                     // string
    caller_name,                    // string
    download_file_path,             // string
    http_custom_headers,            // string
    cost_policy,                    // uint32 (see download_cost_policy enum)
    security_flags,                 // uint32
    callback_freq_percent,          // uint32
    callback_freq_seconds,          // uint32
    no_progress_timeout_seconds,    // uint32
    use_foreground_priority,        // bool
    blocking_mode,                  // bool
    network_token,                  // bool

    // Available beginning in Windows 20H1 (build 19041)
    correlation_vector,             // string
    decryption_info,                // string
    integrity_check_info,           // string
    integrity_check_mandatory,      // bool
    total_size_bytes,               // uint64

    // Available beginning in Windows 21H2 (build 22000)
    disallow_on_cellular,           // bool
    http_custom_auth_headers,       // string

    // Available beginning in Windows 22H2 (build 22621)
    allow_http_to_https_redirect,   // bool
    non_volatile,                   // bool
};

// Values for download_property::cost_policy
enum class download_cost_policy : uint32_t
{
    always = 0,             // download regardless of cost (foreground default)
    unrestricted_network,   // pause download on any metered network
    standard,               // pause download if over or near data limit (background default)
    no_roaming,             // pause download if roaming
    no_surcharge,           // pause download if over data limit
};

class download_property_value
{
friend class details::CDownloadImpl;

public:
    download_property_value();
    ~download_property_value() = default;

    static std::error_code make(const std::string& val, download_property_value& out);
    static std::error_code make(const std::wstring& val, download_property_value& out);
    static std::error_code make(const char* val, download_property_value& out) { return make(std::string(val), out); }
    static std::error_code make(const wchar_t* val, download_property_value& out) { return make(std::wstring(val), out); }
    static std::error_code make(uint32_t val, download_property_value& out);
    static std::error_code make(uint64_t val, download_property_value& out);
    static std::error_code make(bool val, download_property_value& out);

    std::error_code as(bool& val) const noexcept;
    std::error_code as(uint32_t& val) const noexcept;
    std::error_code as(uint64_t& val) const noexcept;
    std::error_code as(std::string& val) const noexcept;
    std::error_code as(std::wstring& val) const noexcept;

private:
    std::shared_ptr<details::CDownloadPropertyValueInternal> _val;
};
} // namespace deliveryoptimization
} // namespace microsoft

#endif // _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_PROPERTY_H
