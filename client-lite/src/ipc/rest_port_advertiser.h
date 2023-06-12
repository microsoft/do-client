// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <errno.h>  // errno
#include <fcntl.h>  // open, write
#include <unistd.h> // getpid
#include <sstream>
#include <gsl/gsl>
#include "do_filesystem.h"
#include "do_persistence.h"
#include "error_macros.h"

// RAII wrapper for creating, writing and deleting the file that will
// contain the port number for the REST interface.
class RestPortAdvertiser
{
public:
    RestPortAdvertiser(uint16_t port)
    {
        // We cannot remove the port file on shutdown because we have already dropped permissions.
        // Clean it up now when we are still running as root.
        _DeleteOlderPortFiles();

        std::stringstream ss;
        ss << docli::GetRuntimeDirectory() << '/' << _restPortFileNamePrefix << '.' << getpid();
        _outFilePath = ss.str();

        // If file already exists, it is truncated. Probably from an old instance that terminated abnormally.
        // Allow file to only be read by others.
        int fd = open(_outFilePath.data(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IRGRP | S_IROTH);
        if (fd == -1)
        {
            THROW_HR_MSG(E_FAIL, "Failed to open file %s, errno: %d", _outFilePath.data(), errno);
        }

        try
        {
            const auto writeStr = std::to_string(port) + '\n';
            const auto cbWrite = gsl::narrow_cast<ssize_t>(writeStr.size() * sizeof(char));

            const ssize_t written = write(fd, writeStr.data(), cbWrite);
            if (written != cbWrite)
            {
                THROW_HR_MSG(E_FAIL, "Failed to write port, written: %zd, errno: %d", written, errno);
            }
        }
        catch (...)
        {
            (void)close(fd);
            throw;
        }
        (void)close(fd);
    }

    const std::string& OutFilePath() const { return _outFilePath; }

private:
    void _DeleteOlderPortFiles() try
    {
        auto& runtimeDirectory = docli::GetRuntimeDirectory();
        for (fs::directory_iterator itr(runtimeDirectory); itr != fs::directory_iterator(); ++itr)
        {
            auto& dirEntry = itr->path();
            if (dirEntry.filename().string().find(_restPortFileNamePrefix) != std::string::npos)
            {
                std::error_code ec;
                fs::remove(dirEntry, ec);
                if (ec)
                {
                    DoLogWarning("Failed to delete old port file (%d, %s) %s", ec.value(), ec.message().data(), dirEntry.string().data());
                }
            }
        }
    } CATCH_LOG()

    std::string _outFilePath;
    static constexpr const char* const _restPortFileNamePrefix = "restport";
};
