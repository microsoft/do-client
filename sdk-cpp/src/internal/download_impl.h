#ifndef _DELIVERY_OPTIMIZATION_DOWNLOAD_IMPL_H
#define _DELIVERY_OPTIMIZATION_DOWNLOAD_IMPL_H

#include <string>

#include "download_interface.h"
#include "do_download_status.h"

#if defined(DO_INTERFACE_COM)
#include <wrl.h>

#include "do_download_property.h"

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

    int32_t Start() override;
    int32_t Pause() override;
    int32_t Resume() override;
    int32_t Finalize() override;
    int32_t Abort() override;

    int32_t GetStatus(download_status& status) override;

#if DO_INTERFACE_COM
    int32_t SetCallback(const download_property_value::status_callback_t& callback, download& download) override;

    int32_t GetProperty(download_property key, download_property_value& value) override;
    int32_t SetProperty(download_property key, const download_property_value& val) override;
#endif

private:
    int32_t _Initialize(const std::string& uri, const std::string& downloadFilePath);

#if defined(DO_INTERFACE_COM)
    static int32_t _SetPropertyHelper(IDODownload& download, download_property key, const download_property_value& val);
    int32_t _GetPropertyHelper(download_property key, download_property_value& value);

    Microsoft::WRL::ComPtr<IDODownload> _spDownload;
#elif defined(DO_INTERFACE_REST)
    int32_t _DownloadOperationCall(const std::string& type);

    std::string _id;
#endif
    
    int32_t _codeInit;
};

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
#endif
