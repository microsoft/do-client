// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_common.h"
#include "do_guid.h"

#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "string_ops.h"

GUID CreateNewGuid()
{
    static_assert(sizeof(GUID) == boost::uuids::uuid::static_size());
    boost::uuids::uuid id = boost::uuids::random_generator()();
    GUID newGuid;
    memcpy(&newGuid, &id, sizeof(newGuid));
    return newGuid;
}

bool StringToGuid(PCSTR guidStr, GUID* guidVal)
{
    constexpr size_t GUIDSTR_MIN = GUIDSTR_MAX - 2 - 1; // without braces and null-terminator
    constexpr int GUID_SEGMENTS = 11;
    const size_t len = strlen(guidStr);
    if ((GUIDSTR_MIN <= len) && (len <= (GUIDSTR_MAX - 1)))
    {
        std::string localGuidStr(guidStr);
        StringCleanup(localGuidStr, "{}");
        boost::algorithm::to_lower(localGuidStr);
        if (localGuidStr.size() == GUIDSTR_MIN)
        {
            GUID tempGuid;
            const int ret = StringScanf(localGuidStr.data(), "%8x-%4hx-%4hx-%2hhx%2hhx-%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
                &tempGuid.Data1, &tempGuid.Data2, &tempGuid.Data3,
                &tempGuid.Data4[0], &tempGuid.Data4[1], &tempGuid.Data4[2], &tempGuid.Data4[3],
                &tempGuid.Data4[4], &tempGuid.Data4[5], &tempGuid.Data4[6], &tempGuid.Data4[7]);
            if (ret == GUID_SEGMENTS)
            {
                assign_to_opt_param(guidVal, tempGuid);
                return true;
            }
        }
    }
    return false;
}

std::string GuidToString(REFGUID guid)
{
    std::array<char, GUIDSTR_MAX> guidStr = {};
    StringPrintf(guidStr.data(), guidStr.size(), "%08x-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x",
        guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
        guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return guidStr.data();
}
