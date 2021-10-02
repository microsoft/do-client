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

    virtual int32_t Init(const std::string& uri, const std::string& downloadFilePath) noexcept = 0;

    virtual int32_t Start() noexcept = 0;
    virtual int32_t Pause() noexcept = 0;
    virtual int32_t Resume() noexcept = 0;
    virtual int32_t Finalize() noexcept = 0;
    virtual int32_t Abort() noexcept = 0;

    virtual int32_t GetStatus(download_status& status) noexcept = 0;

    virtual int32_t GetProperty(download_property key, download_property_value& value) noexcept = 0;
    virtual int32_t SetProperty(download_property key, const download_property_value& val) noexcept = 0;
    virtual int32_t SetCallback(const download_property_value::status_callback_t& callback, download& download) noexcept = 0;
};
} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
#endif
