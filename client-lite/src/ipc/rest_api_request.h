#pragma once

#include <memory>
#include <cpprest/http_msg.h>
#include <cpprest/json.h>
#include "rest_api_parser.h"

class DownloadManager;

class IRestApiRequest
{
public:
    virtual ~IRestApiRequest() = default;
    virtual HRESULT ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser, web::json::value& responseBody) = 0;
};

class RestApiRequestBase
{
public:
    RestApiRequestBase(web::http::http_request clientRequest);
    HRESULT Process(DownloadManager& downloadManager, web::json::value& responseBody);

private:
    std::unique_ptr<IRestApiRequest> _apiRequest;
    RestApiParser _parser;
};

class RestApiCreateRequest : public IRestApiRequest
{
private:
    HRESULT ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser, web::json::value& responseBody) override;
};

class RestApiEnumerateRequest : public IRestApiRequest
{
private:
    HRESULT ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser, web::json::value& responseBody) override;
};

class RestApiDownloadStateChangeRequest : public IRestApiRequest
{
private:
    HRESULT ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser, web::json::value& responseBody) override;
};

class RestApiGetStatusRequest : public IRestApiRequest
{
private:
    HRESULT ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser, web::json::value& responseBody) override;
};

class RestApiGetPropertyRequest : public IRestApiRequest
{
private:
    HRESULT ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser, web::json::value& responseBody) override;
};

class RestApiSetPropertyRequest : public IRestApiRequest
{
private:
    HRESULT ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser, web::json::value& responseBody) override;
};
