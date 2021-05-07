#pragma once

#include "do_noncopyable.h"

// Write-only, binary file wrapper.
// Uses POSIX APIs to provide better error codes than std::fstream/boost::fstream.
class DOFile : DONonCopyable
{
private:
    DOFile(int fd);

public:
    DOFile() = default;

    DOFile(DOFile&& other) noexcept :
        DOFile()
    {
        DO_ASSERT(!IsValid());
        *this = std::move(other);
    }

    DOFile& operator=(DOFile&& other) noexcept
    {
        Close();
        std::swap(_fd, other._fd);
        DO_ASSERT(!other.IsValid());
        return *this;
    }

    static DOFile Create(const std::string& path);
    static DOFile Open(const std::string& path);
    static void Delete(const std::string& path);

    void Append(_In_reads_bytes_(cbData) BYTE* pData, UINT cbData) const;
    void Close();

    operator bool() const noexcept { return IsValid(); }
    bool IsValid() const noexcept { return (_fd >= 0) ;}

private:
    int _fd { -1 };
};
