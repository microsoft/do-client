// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DNONCOPYABLE_H
#define _DELIVERY_OPTIMIZATION_DO_NONCOPYABLE_H

// Handy base class to create non-copyable classes
class CDONoncopyable
{
public:
    CDONoncopyable(const CDONoncopyable&) = delete;
    CDONoncopyable& operator=(const CDONoncopyable&) = delete;

protected:
    CDONoncopyable() {}
};

#endif
