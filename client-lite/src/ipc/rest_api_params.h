#pragma once

#include "download.h"

enum class RestApiParameters
{
    Id,
    Uri,
    DownloadFilePath,
    NoProgressTimeoutSeconds,
    PropertyKey,
};

enum class RestApiParamTypes
{
    UInt,
    String,
};

struct RestApiParam
{
    RestApiParameters paramId;
    const char* stringId;
    DownloadProperty downloadPropertyId;
    RestApiParamTypes type;

    static const RestApiParam& Lookup(RestApiParameters paramId) noexcept;
    static const RestApiParam* Lookup(const char* stringId) noexcept;

    bool IsUnknownDownloadPropertyId() const { return (downloadPropertyId == DownloadProperty::Invalid); }

private:
    static const RestApiParam _knownParams[];
};
