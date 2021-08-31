#pragma once

#include <map>
#include "do_http_packet.h"
#include "rest_api_params.h"

enum class RestApiMethods
{
    Create,
    Enumerate,
    Start,
    Pause,
    Finalize,
    Abort,
    GetStatus,
    GetProperty,
    SetProperty,
};

class RestApiParser
{
public:
    using query_data_t = std::map<const RestApiParam*, std::string>;

    static std::string ParamToString(RestApiParameters param);

    RestApiParser(const std::shared_ptr<microsoft::deliveryoptimization::details::HttpPacket>& request);

    RestApiMethods Method();
    const std::string* QueryStringParam(RestApiParameters param);
    const query_data_t& Query() { return _QueryParams(); }

    std::string GetStringParam(RestApiParameters param);

private:
    void _ParseQueryString();
    const query_data_t& _QueryParams();

private:
    std::shared_ptr<microsoft::deliveryoptimization::details::HttpPacket> _request;

    // All further data members are lazy-init
    RestApiMethods _method;
    query_data_t _queryData;
    bool _methodInitialized { false };
    bool _queryDataInitialized { false };
};
