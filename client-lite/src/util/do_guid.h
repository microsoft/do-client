#pragma once

typedef struct
{
    uint32_t Data1; // Can't use unsigned long here because it is 8bytes on linux
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[8];
} GUID, IID;

using REFGUID = const GUID&;

#ifndef GUIDSTR_MAX
#define GUIDSTR_MAX (1 + 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)
#endif

GUID CreateNewGuid();
bool StringToGuid(PCSTR guidStr, GUID* guidVal = nullptr);
std::string GuidToString(REFGUID guid);
