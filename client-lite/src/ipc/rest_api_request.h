// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <memory>
#include <boost/property_tree/ptree.hpp>
#include "do_http_packet.h"
#include "rest_api_parser.h"

class DownloadManager;

class IRestApiRequest
{
public:
    virtual ~IRestApiRequest() = default;
    virtual HRESULT ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser, boost::property_tree::ptree& responseBody) = 0;
};

class RestApiRequestBase
{
public:
    RestApiRequestBase(const std::shared_ptr<microsoft::deliveryoptimization::details::HttpPacket>& clientRequest);
    HRESULT Process(DownloadManager& downloadManager, boost::property_tree::ptree& responseBody);

private:
    std::unique_ptr<IRestApiRequest> _apiRequest;
    RestApiParser _parser;
};

class RestApiCreateRequest : public IRestApiRequest
{
private:
    HRESULT ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser, boost::property_tree::ptree& responseBody) override;
};

class RestApiEnumerateRequest : public IRestApiRequest
{
private:
    HRESULT ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser, boost::property_tree::ptree& responseBody) override;
};

class RestApiDownloadStateChangeRequest : public IRestApiRequest
{
private:
    HRESULT ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser, boost::property_tree::ptree& responseBody) override;
};

class RestApiGetStatusRequest : public IRestApiRequest
{
private:
    HRESULT ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser, boost::property_tree::ptree& responseBody) override;
};

class RestApiGetPropertyRequest : public IRestApiRequest
{
private:
    HRESULT ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser, boost::property_tree::ptree& responseBody) override;
};

class RestApiSetPropertyRequest : public IRestApiRequest
{
private:
    HRESULT ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser, boost::property_tree::ptree& responseBody) override;
};
