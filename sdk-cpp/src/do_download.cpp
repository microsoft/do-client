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

namespace microsoft
{
namespace deliveryoptimization
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
    std::atomic_bool isCancelled{ false };
    start_and_wait_until_completion(isCancelled, timeOut);
}

void download::start_and_wait_until_completion(const std::atomic_bool& isCancelled, std::chrono::seconds timeOut)
{
    constexpr std::chrono::seconds maxPollTime = 5s;
    std::chrono::milliseconds pollTime = 500ms;
    const auto endTime = std::chrono::system_clock::now() + timeOut;

    start();
    download_status status = get_status();

    bool timedOut = false;
    do
    {
        if (isCancelled)
        {
            break;
        }
        std::this_thread::sleep_for(pollTime);
        if (pollTime < maxPollTime)
        {
            pollTime += 500ms;
        }
        status = get_status();
        timedOut = std::chrono::system_clock::now() >= endTime;
    } while ((status.state() == download_state::created || status.state() == download_state::transferring || status.is_transient_error())
        && !timedOut);

    if (status.state() == download_state::transferred)
    {
        finalize();
    }
    else
    {
        abort();
        if (isCancelled)
        {
            msdod::ThrowException(std::errc::operation_canceled);
        }
        else if (timedOut)
        {
            msdod::ThrowException(std::errc::timed_out);
        }
        else if (status.state() == download_state::paused && !status.is_transient_error())
        {
            assert(status.error_code() != 0);
            msdod::ThrowException(status.error_code());
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


#if (DO_INTERFACE_ID == DO_INTERFACE_ID_COM)
void download::set_property(download_property prop, const download_property_value& val)
{
    if (prop == download_property::callback_interface)
    {
        download_property_value::status_callback_t userCallback;
        val.as(userCallback);

        _download->SetCallback(userCallback, *this);
    }
    else
    {
        _download->SetProperty(prop, val);
    }
}

int32_t download::set_property_nothrow(download_property prop, const download_property_value& val) noexcept
{
    try
    {
        set_property(prop, val);
    }
    catch (const exception& e)
    {
        return e.error_code();
    }
    return 0;
}

download_property_value download::get_property(download_property prop)
{
    return _download->GetProperty(prop);
}
#endif

} // namespace deliveryoptimization
} // namespace microsoft
