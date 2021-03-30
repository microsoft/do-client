#include "do_download.h"

#include <system_error>
#include <cassert>
#include <thread>

#include "do_exceptions.h"
#include "do_exceptions_internal.h"
#include "download_interface.h"
#include "download_impl.h"

namespace msdod = microsoft::deliveryoptimization::details;
using namespace std::chrono_literals; // NOLINT(build/namespaces)

namespace microsoft::deliveryoptimization
{

download::download(const std::string& uri, const std::string& downloadFilePath)
{
    _download = std::make_shared<msdod::CDownloadImpl>(uri, downloadFilePath);
}

download::~download() = default;

void download::start()
{
    _download->Start();
}

void download::pause()
{
    _download->Pause();
}

void download::resume()
{
    _download->Resume();
}

void download::finalize()
{
    _download->Finalize();
}

void download::abort()
{
    _download->Abort();
}

download_status download::get_status() const
{
    return _download->GetStatus();
}

void download::start_and_wait_until_completion(std::chrono::seconds timeOut)
{
    std::atomic_bool isCancelled { false };
    start_and_wait_until_completion(isCancelled, timeOut);
}

void download::start_and_wait_until_completion(const std::atomic_bool& isCancelled, std::chrono::seconds timeOut)
{
    constexpr std::chrono::seconds maxPollTime = 5s;
    std::chrono::milliseconds pollTime = 500ms;
    const auto endTime = std::chrono::system_clock::now() + timeOut;

    start();
    download_status status = get_status();

    bool timedOut = std::chrono::system_clock::now() >= endTime;
    while ((status.state() == download_state::created || status.state() == download_state::transferring || status.is_transient_error()) && !timedOut)
    {
        if (isCancelled)
        {
            abort();
            details::ThrowException(std::errc::operation_canceled);
        }
        std::this_thread::sleep_for(pollTime);
        if (pollTime < maxPollTime)
        {
            pollTime += 500ms;
        }
        status = get_status();
        timedOut = std::chrono::system_clock::now() >= endTime;
    }
    if (status.state() == download_state::transferred)
    {
        finalize();
    }
    else if (status.state() == download_state::paused && !status.is_transient_error())
    {
        assert(status.error_code() != 0);
        details::ThrowException(status.error_code());
    }
    else
    {
        abort();
        if (timedOut)
        {
            details::ThrowException(std::errc::timed_out);
        }
        else
        {
            details::ThrowException(std::errc::connection_refused);
        }
    }
}

void download::download_url_to_path(const std::string& uri, const std::string& downloadFilePath, std::chrono::seconds timeOut)
{
    download oneShotDownload(uri, downloadFilePath);
    oneShotDownload.start_and_wait_until_completion(timeOut);
}

void download::download_url_to_path(const std::string& uri, const std::string& downloadFilePath, const std::atomic_bool& isCancelled, std::chrono::seconds timeOut)
{
    download oneShotDownload(uri, downloadFilePath);
    oneShotDownload.start_and_wait_until_completion(isCancelled, timeOut);
}

} // namespace microsoft::deliveryoptimization
