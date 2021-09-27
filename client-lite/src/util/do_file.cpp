// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_common.h"
#include "do_file.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <cerrno>

DOFile::DOFile(int fd) :
    _fd(fd)
{
    DO_ASSERT(_fd >= 0);
}

DOFile DOFile::Create(const std::string& path)
{
    int fd = open(path.data(), O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    const HRESULT hr = (fd != -1) ? S_OK : HRESULT_FROM_XPLAT_SYSERR(errno);
    DoLogInfoHr(hr, "Create file %s", path.data());
    THROW_IF_FAILED(hr);
    return DOFile{fd};
}

DOFile DOFile::Open(const std::string& path)
{
    int fd = open(path.data(), O_APPEND | O_WRONLY);
    const HRESULT hr = (fd != -1) ? S_OK : HRESULT_FROM_XPLAT_SYSERR(errno);
    DoLogInfoHr(hr, "Open file %s", path.data());
    THROW_IF_FAILED(hr);
    return DOFile{fd};
}

void DOFile::Delete(const std::string& path)
{
    HRESULT hr = S_OK;
    if (remove(path.data()) == -1)
    {
        const auto err = errno;
        if (err != ENOENT)
        {
            hr = HRESULT_FROM_XPLAT_SYSERR(err);
        }
    }
    DoLogInfoHr(hr, "Delete file %s", path.data());
}

void DOFile::Append(_In_reads_bytes_(cbData) BYTE* pData, UINT cbData) const
{
    const ssize_t cbWritten = write(_fd, pData, cbData);
    if (cbWritten == -1)
    {
        THROW_HR(HRESULT_FROM_XPLAT_SYSERR(errno));
    }
    THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_BAD_LENGTH), cbWritten != static_cast<ssize_t>(cbData));
}

void DOFile::Close()
{
    if (_fd != -1)
    {
        if (close(_fd) == -1)
        {
            THROW_HR(HRESULT_FROM_XPLAT_SYSERR(errno));
        }
        _fd = -1;
    }
}
