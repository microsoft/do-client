// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#define INTSAFE_E_ARITHMETIC_OVERFLOW   ((HRESULT)0x80070216L)  // 0x216 = 534 = ERROR_ARITHMETIC_OVERFLOW

inline UINT64 UInt64Add(UINT64 ullAugend, UINT64 ullAddend)
{
    THROW_HR_IF(INTSAFE_E_ARITHMETIC_OVERFLOW, (ullAugend + ullAddend) < ullAugend);
    return (ullAugend + ullAddend);
}

inline UINT64 UInt64Sub(UINT64 ullMinuend, UINT64 ullSubtrahend)
{
    THROW_HR_IF(INTSAFE_E_ARITHMETIC_OVERFLOW, ullMinuend < ullSubtrahend);
    return (ullMinuend - ullSubtrahend);
}
