#pragma once

#include <string>

#include "download_interface.h"
#include "do_download_status.h"

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

private:
    void _DownloadOperationCall(const std::string& type);

    std::string _id;
};

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
