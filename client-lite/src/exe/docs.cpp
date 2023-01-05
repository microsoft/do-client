// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// ----------------------------------------------------------------------------
// The agent is built to run as a daemon (aka service) that starts running as root
// and then drops permissions to 'do' user+group after runtime setup steps are completed.
// ----------------------------------------------------------------------------

#include "do_common.h"

#include <signal.h>
#ifdef DO_DEV_DEBUG
#include <iostream> // std::cin
#endif
#include <chrono>
#include <boost/asio.hpp>
#include <curl/curl.h>
#include "do_event.h"
#include "do_persistence.h"

#include "do_version.h"
namespace msdoutil = microsoft::deliveryoptimization::util::details;

#include "config_manager.h"
#include "do_curl_wrappers.h"
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
#ifdef DO_DEV_DEBUG
        // Workaround for vscode debugger not forwarding ctrl+c to debuggee.
        // Note: When run as a daemon, shutdown is signaled immediately because there is no TTY attached.
        std::thread([this]()
            {
                printf("Type 'quit' and press enter to exit:\n");
                std::string line;
                while (std::getline(std::cin, line) && (line != "quit"));

                DoLogInfo("Received key press {%s}. Initiating shutdown.", line.c_str());
                _shutdownEvent.SetEvent();
            }).detach();
#endif
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

class BoostAsioService
{
public:
    BoostAsioService()
    {
        _workerThread = std::thread{[this] { _io.run(); }};
    }

    ~BoostAsioService()
    {
        _io.stop();
        _workerThread.join();
    }

    boost::asio::io_service& IoService()
    {
        return _io;
    }

private:
    boost::asio::io_service _io;
    boost::asio::io_service::work _work { _io };
    std::thread _workerThread;
};

HRESULT Run() try
{
    InitializeDOPaths();

    CurlGlobalInit curlGlobalInit;
    BoostAsioService asioService;

    ConfigManager clientConfigs;
    auto downloadManager = std::make_shared<DownloadManager>(clientConfigs);
    RestHttpController controller(clientConfigs, downloadManager);

    controller.Start(asioService.IoService());
    DoLogInfo("HTTP controller listening at: %s", controller.ServerEndpoint().data());

    RestPortAdvertiser portAdvertiser(controller.Port());
    DoLogInfo("Port number written to %s", portAdvertiser.OutFilePath().data());
    
#ifndef DO_BUILD_FOR_SNAP
    DropPermissions();
#endif


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
