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

    virtual int32_t Start() = 0;
    virtual int32_t Pause() = 0;
    virtual int32_t Resume() = 0;
    virtual int32_t Finalize() = 0;
    virtual int32_t Abort() = 0;

    virtual int32_t GetStatus(download_status& status) = 0;

#if defined(DO_INTERFACE_COM)
    virtual int32_t GetProperty(download_property key, download_property_value& value) = 0;

    virtual int32_t SetProperty(download_property key, const download_property_value& val) = 0;

    virtual int32_t SetCallback(const download_property_value::status_callback_t& callback, download& download) = 0;
#endif
};
} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
#endif
