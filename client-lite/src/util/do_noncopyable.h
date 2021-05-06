#pragma once

// Handy base class to create non-copyable but movable classes
class DONonCopyable
{
public:
    DONonCopyable(const DONonCopyable&) = delete;
    DONonCopyable& operator=(const DONonCopyable&) = delete;

    DONonCopyable(DONonCopyable&&) noexcept = default;
    DONonCopyable& operator=(DONonCopyable&&) noexcept = default;

protected:
    DONonCopyable() {}
};
