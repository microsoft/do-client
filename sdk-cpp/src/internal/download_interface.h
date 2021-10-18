// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DOWNLOAD_INTERFACE_H
#define _DELIVERY_OPTIMIZATION_DOWNLOAD_INTERFACE_H

#include "do_download_status.h"

#include "do_download_property.h"

class download;

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

class IDownload
{
public:
    virtual ~IDownload() = default;

    virtual std::error_code Init(const std::string& uri, const std::string& downloadFilePath) noexcept = 0;

    virtual std::error_code Start() noexcept = 0;
    virtual std::error_code Pause() noexcept = 0;
    virtual std::error_code Resume() noexcept = 0;
    virtual std::error_code Finalize() noexcept = 0;
    virtual std::error_code Abort() noexcept = 0;

    virtual std::error_code GetStatus(download_status& status) noexcept = 0;

    virtual std::error_code GetProperty(download_property key, download_property_value& value) noexcept = 0;
    virtual std::error_code SetProperty(download_property key, const download_property_value& val) noexcept = 0;
    virtual std::error_code SetCallback(const download_property_value::status_callback_t& callback, download& download) noexcept = 0;
};
} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
#endif
