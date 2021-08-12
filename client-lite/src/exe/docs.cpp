// ----------------------------------------------------------------------------
// The agent is built to run as a daemon (aka service) that starts running as root
// and then drops permissions to 'do' user+group after runtime setup steps are completed.
// ----------------------------------------------------------------------------

#include "do_common.h"

#include <signal.h>

#include <chrono>
#include "do_event.h"
#include "do_persistence.h"

#include "do_version.h"
namespace msdoutil = microsoft::deliveryoptimization::util::details;

#include "config_manager.h"
#include "download_manager.h"
#include "proc_launch_helper.h"
#include "rest_http_controller.h"
#include "rest_port_advertiser.h"

using namespace std::chrono_literals; // NOLINT(build/namespaces) how else should we use chrono literals?

class ProcessController
{
public:
    ProcessController(std::function<void()>&& fnRefreshConfigs)
    {
        _sighupHandler = std::move(fnRefreshConfigs);

        // Do init work like registering signal control handler
        signal(SIGINT, _SignalHandler);
        signal(SIGTERM, _SignalHandler);
        signal(SIGHUP, _SignalHandler);
    }

    void WaitForShutdown(const std::function<bool()>& fnIsIdle)
    {
        constexpr auto idleTimeout = 60s;
        while (true)
        {
            if (_shutdownEvent.Wait(idleTimeout))
            {
                break;
            }

            if (fnIsIdle())
            {
                DoLogInfo("Received idle notification. Initiating shutdown.");
                break;
            }

        }
    }

private:

    static void _SignalHandler(int signalNumber)
    {
        if ((signalNumber == SIGINT) || (signalNumber == SIGTERM))
        {
            DoLogInfo("Received signal (%d). Initiating shutdown.", signalNumber);
            _shutdownEvent.SetEvent();
        }
        else if (signalNumber == SIGHUP)
        {
            DoLogInfo("Received signal (%d). Reloading configurations.", signalNumber);
            _sighupHandler();
        }
    }

    static std::function<void()> _sighupHandler;

    static ManualResetEvent _shutdownEvent;
};

std::function<void()> ProcessController::_sighupHandler;
ManualResetEvent ProcessController::_shutdownEvent;

HRESULT Run() try
{
    InitializeDOPaths();

    ConfigManager clientConfigs;
    auto downloadManager = std::make_shared<DownloadManager>(clientConfigs);
    RestHttpController controller(clientConfigs, downloadManager);

    controller.Start();
    DoLogInfo("HTTP controller listening at: %s", controller.ServerEndpoint().data());

    RestPortAdvertiser portAdvertiser(controller.Port());
    DoLogInfo("Port number written to %s", portAdvertiser.OutFilePath().data());

    DropPermissions();

    DOLog::Init(docli::GetLogDirectory(), DOLog::Level::Verbose);

    DoLogInfo("Started, %s", msdoutil::ComponentVersion().c_str());

    ProcessController procController([&downloadManager]()
    {
        downloadManager->RefreshAdminConfigs();
    });
    procController.WaitForShutdown([&downloadManager]()
    {
        // For now, idle-shutdown mechanism is not applicable when running as a service.
        // The service will be started on boot and will be restarted automatically on failure.
        // SDK can assume docs is running and thus simplifies code for private preview.
        return false;
    });

    DoLogInfo("Exiting...");
    return S_OK;
} CATCH_RETURN()

int main(int argc, char** argv) try
{
    if (msdoutil::OutputVersionIfNeeded(argc, argv))
    {
        return 0;
    }

    const HRESULT hr = LOG_IF_FAILED(Run());

    DOLog::Close();

    printf("Reached end of main, hr: %x\n", hr);
    return hr;
}
catch (...)
{
    const HRESULT hrEx = LOG_CAUGHT_EXCEPTION();
    printf("Caught exception in main, hr: %x\n", hrEx);
    DOLog::Close();
    return hrEx;
}
