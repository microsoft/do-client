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
    _listener.AddHandler([this](RestHttpListener::Message httpRequest)
        {
            _Handler(httpRequest);
        });
}

std::string RestHttpController::ServerEndpoint() const
{
    return _listener.Endpoint();
}

uint16_t RestHttpController::Port() const
{
    return _listener.Port();
}

void RestHttpController::_Handler(RestHttpListener::Message& request)
{
#if 0
    HRESULT hr = S_OK;
    try
    {
        if (_config.RestControllerValidateRemoteAddr() && !_IsValidRemoteAddress(request.remote_address()))
        {
            request.reply(400);
            return;
        }

        auto httpPacket = std::make_shared<microsoft::deliveryoptimization::details::HttpPacket>();
        // httpPacket->body = request.extract_json().get(); // not required at present
        // httpPacket->contentLength // not required
        httpPacket->method = request.method();
        // httpPacket->statusCode // not required
        httpPacket->url = request.request_uri().to_string();

        // shared_ptr because the lambda below needs to be copyable
        auto apiRequest = std::make_shared<RestApiRequestBase>(httpPacket);

        // Handle the request asynchronously and then reply to the client request
        // Note: the tracker is moved to only the last task-based lambda because it
        // will always get executed whereas a value-based lambda can be skipped due
        // to exceptions or cancellations.
        auto tracker = _callTracker.Enter();
        pplx::create_task([this, request, apiRequest]()
            {
                boost::property_tree::ptree responseBody;
                THROW_IF_FAILED(apiRequest->Process(*_downloadManager, responseBody));

                std::stringstream responseBodyStream;
                boost::property_tree::write_json(responseBodyStream, responseBody, false);
                (void)request.reply(web::http::status_codes::OK, responseBodyStream.str());
            }).then([request, tracker = std::move(tracker)](pplx::task<void> t)
            {
                HRESULT hr = S_OK;
                try
                {
                    // get() inside a 'then' handler will re-throw any exception that resulted
                    // from previous task(s). Thus, it allows exceptions to be seen and handled.
                    t.get();
                }
                catch (...)
                {
                    hr = LOG_CAUGHT_EXCEPTION();
                }

                if (FAILED(hr))
                {
                    _OnFailure(request, hr);
                }
            });
    }
    catch (...)
    {
        hr = LOG_CAUGHT_EXCEPTION();
    }

    if (FAILED(hr))
    {
        _OnFailure(request, hr);
    }
#endif
}

bool RestHttpController::_IsValidRemoteAddress(const std::string& addr)
{
    bool fValidAddress = true;
    try
    {
        const auto remoteAddrAsIP = boost::asio::ip::address::from_string(addr);
        if (!remoteAddrAsIP.is_loopback())
        {
            // Log at verbose level to avoid flooding the log (attacker trying to DoS us).
            DoLogVerbose("Request unexpected from non-loopback address: %s", addr.c_str());
            fValidAddress = false;
        }
    }
    catch (...)
    {
        DoLogVerboseHr(docli::ResultFromCaughtException(), "Exception in trying to validate remote address");
        fValidAddress = false;
    }
    return fValidAddress;
}

void RestHttpController::_OnFailure(const RestHttpListener::Message& clientRequest, HRESULT hr) try
{
    std::stringstream responseBody;
    responseBody << "{ \"ErrorCode\": " << hr << " }";
    // (void)clientRequest.reply(_HttpStatusFromHRESULT(hr), responseBody.str());
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
