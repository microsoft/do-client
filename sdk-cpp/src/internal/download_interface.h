// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DOWNLOAD_INTERFACE_H
#define _DELIVERY_OPTIMIZATION_DOWNLOAD_INTERFACE_H

#include "do_download_status.h"

#if defined(DO_INTERFACE_COM)
#include "do_download_property.h"

class download;
#endif

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

    virtual void Start() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual void Finalize() = 0;
    virtual void Abort() = 0;

    virtual download_status GetStatus() = 0;

#if defined(DO_INTERFACE_COM)
    virtual download_property_value GetProperty(download_property key) = 0;

    virtual void SetProperty(download_property key, const download_property_value& val) = 0;

    virtual void SetCallback(const download_property_value::status_callback_t& callback, download& download) = 0;
#endif
};
} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
#endif
