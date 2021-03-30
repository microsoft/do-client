
#pragma once

// Handy base class to create non-copyable classes
class CDONoncopyable
{
public:
    CDONoncopyable(const CDONoncopyable&) = delete;
    CDONoncopyable& operator=(const CDONoncopyable&) = delete;

protected:
    CDONoncopyable() {}
};
