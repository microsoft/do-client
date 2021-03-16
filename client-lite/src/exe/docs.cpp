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
#include "do_noncopyable.h"
#include "proc_launch_helper.h"
#include "rest_http_controller.h"
#include "rest_port_advertiser.h"
#include "trace_sink.h"

using namespace std::chrono_literals; // NOLINT(build/namespaces) how else should we use chrono literals?

class ProcessController : DONonCopyable
{
public:
    ProcessController()
    {
    }

    void SetSignalHandler(const std::function<void()>& fnHandler, int signalNumber)
    {
        if (signalNumber == SIGINT)
        {
            _sigintHandler = std::move(fnHandler);
        }
        else if (signalNumber == SIGTERM)
        {
            _sigtermHandler = std::move(fnHandler);
        }
        else if (signalNumber == SIGHUP)
        {
            _sighupHandler = std::move(fnHandler);
        }
    }

    void ShutDown()
    {
        DoLogInfo("Initiating shutdown");
        _shutdownEvent.SetEvent();
    }

    void Run()
    {
        _RegisterHandlers();
        
        constexpr auto idleTimeout = 60s;
        while (true)
        {
            if (_shutdownEvent.Wait(idleTimeout))
            {
                break;
            }
            
            // Use this opportunity to flush logs periodically
            TraceConsumer::getInstance().Flush();
        }
    }

private:
    void _RegisterHandlers()
    {
        signal(SIGINT, _SignalHandler);
        signal(SIGTERM, _SignalHandler);
        signal(SIGHUP, _SignalHandler);
    }

    static void _SignalHandler(int signalNumber)
    {
        DoLogInfo("Received signal (%d)", signalNumber);
        if (signalNumber == SIGINT)
        {
            _sigintHandler();
        }
        else if (signalNumber == SIGTERM)
        {
            _sigtermHandler();
        }
        else if (signalNumber == SIGHUP)
        {
            _sighupHandler();
        }
    }

    static std::function<void()> _sigintHandler;
    static std::function<void()> _sigtermHandler;
    static std::function<void()> _sighupHandler;

    static ManualResetEvent _shutdownEvent;
};

std::function<void()> ProcessController::_sigintHandler;
std::function<void()> ProcessController::_sigtermHandler;
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

    RETURN_IF_FAILED(TraceConsumer::getInstance().Initialize());
    DoTraceLoggingRegister();

    DoLogInfo("Started, %s", msdoutil::ComponentVersion().c_str());

    ProcessController procController;

    std::function<void()> fnShutdown = [&procController]()
    {
        procController.ShutDown();
    };
    procController.SetSignalHandler(fnShutdown, SIGINT);
    procController.SetSignalHandler(fnShutdown, SIGTERM);

    std::function<void()> fnRefreshConfigs = [&downloadManager]()
    {
        downloadManager->RefreshAdminConfigs();
    };
    procController.SetSignalHandler(fnRefreshConfigs, SIGHUP);

    procController.Run();

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
