#include "do_common.h"
#include "rest_http_controller.h"

#include <boost/asio/ip/address.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <pplx/pplxtasks.h>

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

void RestHttpController::Start()
{
    _listener.Start("http://127.0.0.1");
    _listener.AddHandler(web::http::methods::GET, std::bind(&RestHttpController::_Handler, this, std::placeholders::_1));
    _listener.AddHandler(web::http::methods::POST, std::bind(&RestHttpController::_Handler, this, std::placeholders::_1));
}

std::string RestHttpController::ServerEndpoint() const
{
    return _listener.Endpoint();
}

uint16_t RestHttpController::Port() const
{
    return _listener.Port();
}

void RestHttpController::_Handler(web::http::http_request request)
{
    HRESULT hr = S_OK;
    try
    {
        if (_config.RestControllerValidateRemoteAddr() && !_IsValidRemoteAddress(request.remote_address()))
        {
            request.reply(web::http::status_codes::BadRequest);
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

void RestHttpController::_OnFailure(const web::http::http_request& clientRequest, HRESULT hr) try
{
    std::stringstream responseBody;
    responseBody << "{ \"ErrorCode\": " << hr << " }";
    (void)clientRequest.reply(_HttpStatusFromHRESULT(hr), responseBody.str());
} CATCH_LOG()

web::http::status_code RestHttpController::_HttpStatusFromHRESULT(HRESULT hr)
{
    web::http::status_code status;
    switch (hr)
    {
    case S_OK:
    case S_FALSE:
        status = web::http::status_codes::OK;
        break;
    case E_NOT_SET:
        status = web::http::status_codes::NotFound;
        break;
    case E_OUTOFMEMORY:
        status = web::http::status_codes::ServiceUnavailable;
        break;
    case HRESULT_FROM_WIN32(ERROR_UNHANDLED_EXCEPTION):
        status = web::http::status_codes::InternalError;
        break;
    default:
        status = web::http::status_codes::BadRequest;
        break;
    }
    return status;
}
