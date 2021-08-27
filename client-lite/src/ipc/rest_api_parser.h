#pragma once

#include <cpprest/http_msg.h>
#include <cpprest/json.h>
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

    RestApiParser(web::http::http_request request);

    RestApiMethods Method();
    const std::string* QueryStringParam(RestApiParameters param);
    const query_data_t& Query() { return _QueryParams(); }

    std::string GetStringParam(RestApiParameters param);

private:
    void _ParseQueryString();
    const query_data_t& _QueryParams();

private:
    web::http::http_request _request;

    // All further data members are lazy-init
    RestApiMethods _method;
    query_data_t _queryData;
    bool _methodInitialized { false };
    bool _queryDataInitialized { false };
};
