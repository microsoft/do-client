#pragma once

// Handy base class to create non-copyable classes
class DONonCopyable
{
public:
    DONonCopyable(const DONonCopyable&) = delete;
    DONonCopyable& operator=(const DONonCopyable&) = delete;

protected:
    DONonCopyable() {}
};
