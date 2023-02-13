// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DOWNLOAD_IMPL_H
#define _DELIVERY_OPTIMIZATION_DOWNLOAD_IMPL_H

#include "download_interface.h"

#if defined(DO_INTERFACE_COM)
#include <wrl.h>

// Future: delete the local copy of deliveryoptimization.h and require Windows SDK 22621+
#include <deliveryoptimization.h> // IDODownload, etc.
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
    CDownloadImpl() = default;

    std::error_code Init(const std::string& uri, const std::string& downloadFilePath) noexcept override;

    std::error_code Start() noexcept override;
    std::error_code Pause() noexcept override;
    std::error_code Resume() noexcept override;
    std::error_code Finalize() noexcept override;
    std::error_code Abort() noexcept override;

    std::error_code GetStatus(download_status& status) noexcept override;
    std::error_code SetStatusCallback(const status_callback_t& callback, download& download) noexcept override;
    std::error_code SetStreamCallback(const output_stream_callback_t& callback) noexcept override;
    std::error_code GetProperty(download_property key, download_property_value& value) noexcept override;
    std::error_code SetProperty(download_property key, const download_property_value& val) noexcept override;
    std::error_code SetRanges(const download_range* ranges, size_t count) noexcept override;

private:
#if defined(DO_INTERFACE_COM)
    Microsoft::WRL::ComPtr<IDODownload> _spDownload;
    std::unique_ptr<DO_DOWNLOAD_RANGES_INFO> _spRanges;
#elif defined(DO_INTERFACE_REST)
    std::error_code _DownloadOperationCall(const std::string& type) noexcept;

    std::string _id;
#endif
};

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft

#endif // _DELIVERY_OPTIMIZATION_DOWNLOAD_IMPL_H
