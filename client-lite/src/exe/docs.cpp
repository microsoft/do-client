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
#include "trace_sink.h"

using namespace std::chrono_literals; // NOLINT(build/namespaces) how else should we use chrono literals?

class ProcessController
{
public:
    ProcessController()
    {
        // Do init work like registering signal control handler
        signal(SIGINT, _SignalHandler);
        signal(SIGTERM, _SignalHandler);
        signal(SIGHUP, _SignalHandler);
    }

    void WaitForShutdown(const std::function<void()>& fnRefreshConfigs)
    {
        constexpr auto idleTimeout = 60s;
        while (true)
        {
            /* For now, idle-shutdown mechanism is not applicable when running as a service.
            The service will be started on boot and will be restarted automatically on failure.
            SDK can assume docs is running and thus simplifies code for private preview. */
            if (_shutdownEvent.Wait(idleTimeout))
            {
                break;
            }
            if (_refreshEvent.IsSignaled())
            {
                fnRefreshConfigs();
                _refreshEvent.ResetEvent();
            }

            // Use this opportunity to flush logs periodically
            TraceConsumer::getInstance().Flush();

        }
    }

private:
    static void _SignalHandler(int signalNumber)
    {
        if ((signalNumber == SIGINT) || (signalNumber == SIGTERM))
        {
            DoLogInfo("Received signal %d. Initiating shutdown.", signalNumber);
            _shutdownEvent.SetEvent();
        }
        if (signalNumber == SIGHUP)
        {
            DoLogInfo("Received signal %d to reload configurations.", signalNumber);
            _refreshEvent.SetEvent();
        }
    }

    static ManualResetEvent _refreshEvent;
    static ManualResetEvent _shutdownEvent;
};

ManualResetEvent ProcessController::_shutdownEvent;
ManualResetEvent ProcessController::_refreshEvent;

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

    RETURN_IF_FAILED(TraceConsumer::getInstance().Initialize());
    DoTraceLoggingRegister();

    DoLogInfo("Started, %s", msdoutil::ComponentVersion().c_str());

    ProcessController procController;
    procController.WaitForShutdown([&downloadManager]()
    {
        downloadManager->RefreshConfigs();
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

    DoTraceLoggingUnregister();
    TraceConsumer::getInstance().Finalize();

    printf("Reached end of main, hr: %x\n", hr);
    return hr;
}
catch (...)
{
    const HRESULT hrEx = LOG_CAUGHT_EXCEPTION();
    printf("Caught exception in main, hr: %x\n", hrEx);
    DoTraceLoggingUnregister();
    TraceConsumer::getInstance().Finalize();
    return hrEx;
}
