#include "do_common.h"
#include "rest_http_controller.h"

#include <boost/asio/ip/address.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "config_manager.h"
#include "do_http_packet.h"
#include "download_manager.h"
#include "rest_api_request.h"

RestHttpController::RestHttpController(ConfigManager& config, std::shared_ptr<DownloadManager> downloadManager) :
    _config(config),
    _downloadManager(std::move(downloadManager))
{
}

RestHttpController::~RestHttpController()
{
    _listener.Stop();
    (void)_callTracker.Wait();
}

void RestHttpController::Start(boost::asio::io_service& ioService)
{
    _listener.Start(ioService);
    _listener.AddHandler(std::bind(&RestHttpController::_Handler, this, std::placeholders::_1, std::placeholders::_2));
}

std::string RestHttpController::ServerEndpoint() const
{
    return _listener.Endpoint();
}

uint16_t RestHttpController::Port() const
{
    return _listener.Port();
}

void RestHttpController::_Handler(const std::shared_ptr<microsoft::deliveryoptimization::details::HttpPacket>& packet,
    HttpListenerConnection& conn)
{
    HRESULT hr = S_OK;
    std::stringstream responseBodyStream;
    try
    {
        if (_config.RestControllerValidateRemoteAddr())
        {
            const auto addr = conn.RemoteEndpoint().address();
            if (!addr.is_loopback())
            {
                DoLogVerbose("Request unexpected from non-loopback address: %s", addr.to_string().c_str());
                hr = E_INVALIDARG;
            }
        }

        boost::property_tree::ptree responseBody;
        if (SUCCEEDED(hr))
        {
            hr = RestApiRequestBase{packet}.Process(*_downloadManager, responseBody);
        }
        if (SUCCEEDED(hr))
        {
            std::stringstream ss;
            boost::property_tree::write_json(responseBodyStream, responseBody, false);
        }
    }
    catch (...)
    {
        hr = LOG_CAUGHT_EXCEPTION();
    }

    if (SUCCEEDED(hr))
    {
        conn.Reply(200, responseBodyStream.str());
    }
    else
    {
        _OnFailure(conn, hr);
    }
}

void RestHttpController::_OnFailure(HttpListenerConnection& conn, HRESULT hr) try
{
    std::stringstream responseBody;
    responseBody << "{ \"ErrorCode\": " << hr << " }";
    conn.Reply(_HttpStatusFromHRESULT(hr), responseBody.str());
} CATCH_LOG()

UINT RestHttpController::_HttpStatusFromHRESULT(HRESULT hr)
{
    UINT status;
    switch (hr)
    {
    case S_OK:
    case S_FALSE:
        status = 200;
        break;
    case E_NOT_SET:
        status = 404;
        break;
    case E_OUTOFMEMORY:
        status = 503;
        break;
    case HRESULT_FROM_WIN32(ERROR_UNHANDLED_EXCEPTION):
        status = 500;
        break;
    default:
        status = 400;
        break;
    }
    return status;
}
