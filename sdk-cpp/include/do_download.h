#ifndef _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_H
#define _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_H

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include "do_download_status.h"

#include "do_download_property.h"

namespace microsoft
{
namespace deliveryoptimization
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

    void start_and_wait_until_completion(std::chrono::seconds timeoutSecs = std::chrono::hours(24));
    void start_and_wait_until_completion(const std::atomic_bool& isCancelled, std::chrono::seconds timeoutSecs = std::chrono::hours(24));
    static void download_url_to_path(const std::string& uri, const std::string& downloadFilePath, std::chrono::seconds timeoutSecs = std::chrono::hours(24));
    static void download_url_to_path(const std::string& uri, const std::string& downloadFilePath, const std::atomic_bool& isCancelled, std::chrono::seconds timeoutSecs = std::chrono::hours(24));

    /*
    For devices running windows before 20H1, dosvc exposed a now-deprecated com interface for setting certain download properties.
    After 20H1, these properties were added to newer com interface, which this SDK is using.
    Attempting to set a download property on a version of windows earlier than 20H1 will not set the property and throw an exception with error code msdo::errc::do_e_unknown_property_id
    */
    void set_property(download_property key, const download_property_value& value);
    download_property_value get_property(download_property key);

    int32_t set_property_nothrow(download_property key, const download_property_value& value) noexcept;

private:
    std::shared_ptr<details::IDownload> _download;
};

} // namespace deliveryoptimization
} // namespace microsoft
#endif // _DELIVERY_OPTIMIZATION_DO_DOWNLOAD_H
