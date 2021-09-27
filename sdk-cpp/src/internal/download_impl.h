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
    CDownloadImpl(const std::string& uri, const std::string& downloadFilePath);

    void Start() override;
    void Pause() override;
    void Resume() override;
    void Finalize() override;
    void Abort() override;

    download_status GetStatus() override;
    download_property_value GetProperty(download_property key) override;
    void SetProperty(download_property key, const download_property_value& val) override;
    void SetCallback(const download_property_value::status_callback_t& callback, download& download) override;

private:
#if defined(DO_INTERFACE_COM)
    static void _SetPropertyHelper(IDODownload& download, download_property key, const download_property_value& val);
    download_property_value _GetPropertyHelper(download_property key);

    Microsoft::WRL::ComPtr<IDODownload> _spDownload;
#elif defined(DO_INTERFACE_REST)
    void _DownloadOperationCall(const std::string& type);

    std::string _id;
#endif
};

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
#endif
