#include "do_common.h"
#include "rest_api_params.h"

#include "string_ops.h"

#define INSERT_REST_API_PARAM(_p)     RestApiParameters::_p, #_p

// Maintain the same order as in RestApiParameters
const RestApiParam RestApiParam::_knownParams[] =
{
    { INSERT_REST_API_PARAM(Id), DownloadProperty::Id, RestApiParamTypes::String },
    { INSERT_REST_API_PARAM(Uri), DownloadProperty::Uri, RestApiParamTypes::String },
    { INSERT_REST_API_PARAM(DownloadFilePath), DownloadProperty::LocalPath, RestApiParamTypes::String },
    { INSERT_REST_API_PARAM(NoProgressTimeoutSeconds), DownloadProperty::NoProgressTimeoutSeconds, RestApiParamTypes::UInt },
    { INSERT_REST_API_PARAM(PropertyKey), DownloadProperty::Invalid, RestApiParamTypes::String },
};

const RestApiParam& RestApiParam::Lookup(RestApiParameters paramId) noexcept
{
    return _knownParams[static_cast<size_t>(paramId)];
}

const RestApiParam* RestApiParam::Lookup(const char* stringId) noexcept
{
    for (const auto& param : _knownParams)
    {
        if (StringCompareCaseInsensitive(param.stringId, stringId) == 0)
        {
            return &param;
        }
    }
    DoLogWarning("%s is not a known REST API param", stringId);
    return nullptr;
}
