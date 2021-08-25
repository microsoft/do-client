#include "do_download.h"

#include <system_error>
#include <cassert>
#include <thread>

#include "do_exceptions.h"
#include "do_exceptions_internal.h"
#include "download_interface.h"
#include "download_rest.h"

namespace msdod = microsoft::deliveryoptimization::details;
using namespace std::chrono_literals; // NOLINT(build/namespaces)

namespace microsoft
{
namespace deliveryoptimization
{

download::download(const std::string& uri, const std::string& downloadFilePath)
{
    _download = std::make_shared<msdod::CDownloadRest>(uri, downloadFilePath);
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

void download::download_url_to_path(const std::string& uri, const std::string& downloadFilePath, std::chrono::seconds timeOut)
{
    std::atomic_bool isCancelled { false };
    download_url_to_path(uri, downloadFilePath, isCancelled, timeOut);
}

// Ensures download is finalized/aborted in the agent to avoid stale download objects.
// Future: We could expect callers to explicitly opt-in to persist downloads in the agent and
//   use that as an indicator to perform/skip the cleanup directly in ~download().
class download_cleanup_call
{
private:
    download* _ptr { nullptr };

public:
    explicit download_cleanup_call(download* pObj) :
        _ptr(pObj)
    {
    }

    ~download_cleanup_call()
    {
        if (_ptr != nullptr)
        {
            try
            {
                const auto state = _ptr->get_status().state();
                if (state == download_state::transferred)
                {
                    _ptr->finalize();
                }
                else if ((state != download_state::aborted) && (state != download_state::finalized))
                {
                    _ptr->abort();
                }
            }
            catch (...)
            {
            }
        }
    }
};

void download::download_url_to_path(const std::string& uri, const std::string& downloadFilePath, const std::atomic_bool& isCancelled, std::chrono::seconds timeOut)
{
    constexpr std::chrono::seconds maxPollTime = 5s;
    std::chrono::milliseconds pollTime = 500ms;
    const auto endTime = std::chrono::system_clock::now() + timeOut;

    download oneShotDownload(uri, downloadFilePath);
    download_cleanup_call scopedCleanup{&oneShotDownload};

    oneShotDownload.start();

    download_status status;
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
        status = oneShotDownload.get_status();
        timedOut = std::chrono::system_clock::now() >= endTime;
    } while ((status.state() == download_state::created || status.state() == download_state::transferring || status.is_transient_error())
        && !timedOut);

    if (status.state() == download_state::transferred)
    {
        // nothing to do, download_cleanup_call invokes finalize()
    }
    else
    {
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

} // namespace deliveryoptimization
} // namespace microsoft
