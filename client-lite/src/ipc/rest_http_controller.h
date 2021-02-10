#pragma once

#include <cpprest/http_msg.h>
#include "rest_http_listener.h"
#include "waitable_counter.h"

class ConfigManager;
class DownloadManager;
class RestApiRequestBase;

// Controller for the REST-over-HTTP interface in DO client.
// This interface is used as the inter-process communication
// mechanism for our clients to create and manage download requests.
class RestHttpController
{
public:
    RestHttpController(ConfigManager& config, std::shared_ptr<DownloadManager> downloadManager);
    ~RestHttpController();

    void Start();
    std::string ServerEndpoint() const;
    uint16_t Port() const;

private:
    void _Handler(web::http::http_request request);
    static bool _IsValidRemoteAddress(const std::string& addr);
    static void _OnFailure(const web::http::http_request& clientRequest, HRESULT hr);
    static web::http::status_code _HttpStatusFromHRESULT(HRESULT hr);

private:
    ConfigManager& _config;
    std::shared_ptr<DownloadManager> _downloadManager;
    RestHttpListener _listener;
    WaitableCounter _callTracker;
};
