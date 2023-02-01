// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_H
#define _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_H

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include "do_download_status.h"
#include "do_download_property.h"

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{
class IDownload;
}

class download
{
public:
    ~download();

    static std::error_code make(const std::string& uri, const std::string& downloadFilePath, std::unique_ptr<download>& out) noexcept;

    std::error_code start() noexcept;
    std::error_code pause() noexcept;
    std::error_code resume() noexcept;
    std::error_code finalize() noexcept;
    std::error_code abort() noexcept;
    std::error_code get_status(download_status& status) noexcept;
    std::error_code set_status_callback(status_callback_t callback) noexcept;

    std::error_code start_and_wait_until_completion(std::chrono::seconds timeoutSecs = std::chrono::hours(24)) noexcept;
    std::error_code start_and_wait_until_completion(const std::atomic_bool& isCancelled, std::chrono::seconds timeoutSecs = std::chrono::hours(24)) noexcept;
    static std::error_code download_url_to_path(const std::string& uri, const std::string& downloadFilePath, std::chrono::seconds timeoutSecs = std::chrono::hours(24)) noexcept;
    static std::error_code download_url_to_path(const std::string& uri, const std::string& downloadFilePath, const std::atomic_bool& isCancelled, std::chrono::seconds timeoutSecs = std::chrono::hours(24)) noexcept;

    /*
    Certain properties are not supported on older versions of Windows, resulting in
    msdo::errc::do_e_unknown_property_id from the following methods. See do_download_property.h.
    */
    std::error_code set_property(download_property prop, const download_property_value& value) noexcept;
    std::error_code get_property(download_property prop, download_property_value& value) noexcept;

    template <typename T>
    std::error_code set_property(download_property prop, const T& value) noexcept
    {
        download_property_value propVal;
        std::error_code ec = download_property_value::make(value, propVal);
        if (!ec)
        {
            ec = set_property(prop, propVal);
        }
        return ec;
    }

    template <typename T>
    std::error_code get_property(download_property prop, T& value) noexcept
    {
        value = {};
        download_property_value propVal;
        std::error_code ec = get_property(prop, propVal);
        if (!ec)
        {
            ec = propVal.as(value);
        }
        return ec;
    }

    std::error_code set_cost_policy(download_cost_policy value) noexcept
    {
        return set_property(download_property::cost_policy, static_cast<uint32_t>(value));
    }

    std::error_code set_security_flags(download_security_flags value) noexcept
    {
        return set_property(download_property::security_flags, static_cast<uint32_t>(value));
    }

private:
    download();

    std::shared_ptr<details::IDownload> _download;
};

} // namespace deliveryoptimization
} // namespace microsoft

#endif // _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_H
