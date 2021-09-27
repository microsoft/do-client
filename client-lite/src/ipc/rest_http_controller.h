// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

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

    void Start(boost::asio::io_service& ioService);
    std::string ServerEndpoint() const;
    uint16_t Port() const;

private:
    void _HttpListenerCallback(const std::shared_ptr<microsoft::deliveryoptimization::details::HttpPacket>& packet,
        HttpListenerConnection& conn);
    static void _OnFailure(HttpListenerConnection& conn, HRESULT hr);
    static UINT _HttpStatusFromHRESULT(HRESULT hr);

private:
    ConfigManager& _config;
    std::shared_ptr<DownloadManager> _downloadManager;
    RestHttpListener _listener;
    WaitableCounter _callTracker;
};
