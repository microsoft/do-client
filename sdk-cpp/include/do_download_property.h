// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_PROPERTY_H
#define _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_PROPERTY_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

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
class download;
class download_status;

/*
For REST interface, these download properties are not yet supported
SDK will throw/return msdo::errc::e_notimpl if attempting to set/get a property
*/
enum class download_property
{
    id,                             // std::string
    uri,                            // std::string
    catalog_id,                     // std::string
    caller_name,                    // std::string
    download_file_path,             // std::string
    http_custom_headers,            // std::string
    cost_policy,                    // uint32
    security_flags,                 // uint32
    callback_freq_percent,          // uint32
    callback_freq_seconds,          // uint32
    no_progress_timeout_seconds,    // uint32
    use_foreground_priority,        // bool
    blocking_mode,                  // bool
    callback_interface,             // void*, but used for storing lambda expressions
    stream_interface,               // void*
    security_context,               // byte array
    network_token,                  // bool
    correlation_vector,             // std::string
    decryption_info,                // std::string
    integrity_check_info,           // std::string
    integrity_check_mandatory,      // boolean
    total_size_bytes,               // uint64

    // For the COM interface, the following properties are available only in Windows 21H2 (Build Number 22000) and beyond
    disallow_on_cellular,           // bool
    http_custom_auth_headers,       // std::string
};

class download_property_value
{

/*
CDownloadImpl is declared as a friend class because it needs to access the platform-specific native value for download_property_value
The type of the native value is defined in CDownloadPropertyValueInternal, because DO header files are platform agnostic
This is so any user of the SDK does not have to worry about supplying platform specific compile definitions to use the SDK
*/
friend class details::CDownloadImpl;

public:
    using status_callback_t = std::function<void(download&, download_status&)>;

    download_property_value();
    ~download_property_value() = default;

    static std::error_code make(const std::string& val, download_property_value& out);
    static std::error_code make(uint32_t val, download_property_value& out);
    static std::error_code make(uint64_t val, download_property_value& out);
    static std::error_code make(bool val, download_property_value& out);
    static std::error_code make(std::vector<unsigned char>& val, download_property_value& out);
    static std::error_code make(const status_callback_t& val, download_property_value& out);

    std::error_code as(bool& val) const noexcept;
    std::error_code as(uint32_t& val) const noexcept;
    std::error_code as(uint64_t& val) const noexcept;
    std::error_code as(std::string& val) const noexcept;
    std::error_code as(std::vector<unsigned char>& val) const noexcept;
    std::error_code as(status_callback_t& val) const noexcept;

private:
    std::shared_ptr<details::CDownloadPropertyValueInternal> _val;
};
} // namespace deliveryoptimization
} // namespace microsoft

#endif // _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_PROPERTY_H
