#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include "do_download_status.h"

namespace microsoft::deliveryoptimization
{

namespace details
{
class IDownload;
}

class download
{
public:
    download(const std::string& uri, const std::string& downloadFilePath);
    ~download();

    void start();
    void pause();
    void resume();
    void finalize();
    void abort();

    download_status get_status() const;

    static void download_url_to_path(const std::string& uri, const std::string& downloadFilePath, std::chrono::seconds timeoutSecs = std::chrono::hours(24));
    static void download_url_to_path(const std::string& uri, const std::string& downloadFilePath, const std::atomic_bool& isCancelled, std::chrono::seconds timeoutSecs = std::chrono::hours(24));

private:
    std::shared_ptr<details::IDownload> _download;
};

} // namespace microsoft::deliveryoptimization
