#ifndef _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_PROPERTY_H
#define _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_PROPERTY_H

//TODO(jimson): Callers may not have defined these compile definitions, as a result their builds may fail if the definition is not set when using the SDK
//Look into removing platform specific header files from the SDK installation
#if (DO_INTERFACE_ID == DO_INTERFACE_ID_COM)

#include <OAIdl.h>
#include <functional>
#include <string>
#include <vector>

namespace microsoft
{
namespace deliveryoptimization
{

class download;
class download_status;

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
    // Fe and beyond
    disallow_on_cellular,           // bool
    http_custom_auth_headers,       // std::string
};

class download_property_value
{

public:
    using native_type = VARIANT;
    using status_callback_t = std::function<void(download&, download_status&)>;

    download_property_value() = default;

    explicit download_property_value(const std::string& val);
    explicit download_property_value(UINT val);
    explicit download_property_value(UINT64 val);
    explicit download_property_value(bool val);
    explicit download_property_value(std::vector<unsigned char>& val); // DODownloadProperty_SecurityContext

    explicit download_property_value(const status_callback_t& val);

    ~download_property_value();

    download_property_value(const download_property_value& rhs);
    download_property_value& operator=(download_property_value copy);
    download_property_value(download_property_value&& rhs) noexcept;

    friend void swap(download_property_value& first, download_property_value& second) noexcept
    {
        std::swap(first._var, second._var);
        std::swap(first._callback, second._callback);
    }

    void as(bool& val) const;
    void as(UINT& val) const;
    void as(UINT64& val) const;
    void as(std::string& val) const;

    void as(status_callback_t& val) const;
    void as(std::vector<unsigned char>& val) const;

    const native_type& native_value() const;

private:
    native_type _var;
    status_callback_t _callback;
};
}
}
#endif // Windows
#endif // _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_PROPERTY_H
