// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DOWNLOAD_IMPL_H
#define _DELIVERY_OPTIMIZATION_DOWNLOAD_IMPL_H

#include <string>

#include "download_interface.h"
#include "do_download_property.h"
#include "do_download_status.h"

#if defined(DO_INTERFACE_COM)
#include <wrl.h>

#include "deliveryoptimization.h" // Fwd declaration of IDODownload doesn't work well w/ chromium builds
#endif

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{
class CDownloadImpl : public IDownload
{
public:
    // TODO(jimson): Could make the default constructor private and have static make methods accept a reference
    // Callers would need to initialize a pointer to CDownloadImpl, and call Init(download, uri, downloadFilePath);
    CDownloadImpl() = default;
    
    int32_t Init(const std::string& uri, const std::string& downloadFilePath) noexcept;

    int32_t Start() noexcept override;
    int32_t Pause() noexcept override;
    int32_t Resume() noexcept override;
    int32_t Finalize() noexcept override;
    int32_t Abort() noexcept override;

    int32_t GetStatus(download_status& status) noexcept override;
    int32_t GetProperty(download_property key, download_property_value& value) noexcept override;
    int32_t SetProperty(download_property key, const download_property_value& val) noexcept override;
    int32_t SetCallback(const download_property_value::status_callback_t& callback, download& download) noexcept override;

private:

#if defined(DO_INTERFACE_COM)
    static int32_t _SetPropertyHelper(IDODownload& download, download_property key, const download_property_value& val) noexcept;
    int32_t _GetPropertyHelper(download_property key, download_property_value& value) noexcept;

    Microsoft::WRL::ComPtr<IDODownload> _spDownload;
#elif defined(DO_INTERFACE_REST)
    int32_t _DownloadOperationCall(const std::string& type) noexcept;

    std::string _id;
#endif
};

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
#endif
