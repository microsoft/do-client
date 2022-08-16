// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <uuid/uuid.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>

#include <deliveryoptimization/do_download.h>
namespace msdo = microsoft::deliveryoptimization;

#include "do_hash.h"
#include "do_log.h"
#include "do_plugin_exception.h"
#include "do_string_util.h"
#include "do_version.h"
namespace msdoutil = microsoft::deliveryoptimization::util::details;

const char* const g_cachePath = DO_PLUGIN_APT_CACHE_PATH;

enum APTMsgCodes
{
    Capabilities = 100,
    Status = 102,
    UriStart = 200,
    UriDone = 201,
    UriFailure = 400,

    UriAcquire = 600,
    Configuration = 601
};

const std::map<int, std::string> g_APTMsgStrings =
{
    // 1xx-4xx codes are sent by us to APT
    { APTMsgCodes::Capabilities, "Capabilities" },
    { APTMsgCodes::Status, "Status"},
    { APTMsgCodes::UriStart, "URI Start"},
    { APTMsgCodes::UriDone, "URI Done"},
    { APTMsgCodes::UriFailure, "URI Failure"},

    // 6xx codes are sent to us from APT
    { APTMsgCodes::UriAcquire, "URI Acquire"},
    { APTMsgCodes::Configuration, "Configuration"}
};

// Note: keyValues param uses std::vector to preserve order of items
std::string ConstructAPTMessage(APTMsgCodes code, const std::vector<std::string>& keyValues)
{
    if ((keyValues.size() % 2) != 0)
    {
        throw DOPluginException("Unexpected number of keyValues for APT msg: %d", keyValues.size());
    }

    std::stringstream ss;
    ss << code << " " << g_APTMsgStrings.find(code)->second << "\n";
    for (size_t i = 0; i < keyValues.size(); i += 2)
    {
        ss << keyValues[i] << ": " << keyValues[i + 1] << "\n";
    }
    ss << "\n";
    return ss.str();
}

void SendAPTMessage(APTMsgCodes code, const std::vector<std::string>& keyValues)
{
    const std::string msg = ConstructAPTMessage(code, keyValues);
    LogDebug("Sending APT message:\n%s", msg.data());
    // Since the conversation with APT is via stdin/stdout, it is crucial that nothing else gets fed to these file descriptors.
    // So we should not write even debug logs to stdout. Keep in mind that child processes can inherit these file descriptors.
    std::cout << msg << std::flush;
}

void SendUriFailure(const std::string& url, const std::string& msg)
{
    SendAPTMessage(APTMsgCodes::UriFailure, { "URI", url, "Message", msg });
}

void SendUriStart(const std::string& url, size_t size)
{
    SendAPTMessage(APTMsgCodes::UriStart, { "URI", url, "Size", std::to_string(size) });
}

void SendUriDone(const std::string& url, size_t fileSizeBytes, const std::string& filePath,
        const std::vector<std::string>& hashes)
{
    std::vector<std::string> headers{ "URI", url, "Size", std::to_string(fileSizeBytes), "Filename", filePath };
    headers.insert(headers.end(), hashes.begin(), hashes.end());
    SendAPTMessage(APTMsgCodes::UriDone, headers);
}

struct APTCommand
{
    bool eof { false };
    std::string code;
    std::string codeDescription;
    std::unordered_map<std::string, std::string> headers;
};

APTCommand ReceiveAPTMessage()
{
    LogDebug("Waiting for command");

    std::string line;
    APTCommand command;
    do
    {
        if (!std::getline(std::cin, line))
        {
            command.eof = true;
            return command;
        }
     } while (line.empty() || (line == "\n"));

    /* Example:
    600 URI Acquire
    URI: <url>
    Filename: <destination file path>
    Last-Modified: <timestamp of cached file if applicable>
    */

    LogDebug("Got command: %s", line.c_str());

    std::regex rexCommand("(\\d+) (.*)$");
    std::cmatch matches;
    if (std::regex_match(line.c_str(), matches, rexCommand))
    {
        command.code = matches[1].str();
        command.codeDescription = matches[2].str();
    }

    // Parse list of headers, if any
    LogDebug("Waiting for command headers, if any");
    while (std::getline(std::cin, line) && (!line.empty() && (line != "\n")))
    {
        LogDebug("Got command header: %s", line.c_str());

        std::regex rexHeader("([a-zA-Z\\-]+)\\s*:\\s*(.*)");
        if (std::regex_match(line.c_str(), matches, rexHeader))
        {
            auto key = matches[1].str();
            auto value = matches[2].str();
            LogDebug("Key: %s, Value: %s", key.c_str(), value.c_str());
            command.headers[key] = value;
        }
    }
    return command;
}

std::string GenerateUniqueCachePath()
{
    uuid_t guidValue;
    uuid_generate(guidValue);
    char guidStrValue[36 + 1];
    uuid_unparse_lower(guidValue, guidStrValue);

    std::stringstream ss;
    ss << g_cachePath << '/' << guidStrValue << ".tmp";
    return ss.str();
}

void TryDeleteFile(const std::string& path)
{
    if (!path.empty())
    {
        const auto ret = remove(path.data());
        if (ret == -1)
        {
            LogError("Failed to delete file, %d: %s", errno, path.data());
        }
    }
}

struct DownloadResult
{
    size_t fileSizeBytes;
    std::vector<std::string> hashes;
};

DownloadResult SimpleDownload(const std::string& url, const std::string& destFilePath)
{
    LogDebug("Downloading: %s to %s", url.data(), destFilePath.data());
    std::string tempPath;
    try
    {
        tempPath = GenerateUniqueCachePath();
        LogDebug("Using temp file %s for %s", tempPath.data(), destFilePath.data());

        std::error_code errorCode = msdo::download::download_url_to_path(url, tempPath);
        if (errorCode)
        {
            throw DOPluginException("Download hit an error: %d, %s", errorCode.value(), errorCode.message().c_str());
        }
        LogDebug("Download successful");

        if (rename(tempPath.data(), destFilePath.data()) == -1)
        {
            // Future: we could handle EXDEV error (oldpath and newpath are not on the same mounted filesystem) by
            // falling back to a copy operation but we'll implement this only if needed. Right now the fact that
            // tempPath and APT's path is under /var/cache should prevent this error.
            throw DOPluginException("rename failed with %d, %s --> %s", errno, tempPath.data(), destFilePath.data());
        }
        LogDebug("Rename successful: %s --> %s", tempPath.data(), destFilePath.data());
    }
    catch (const std::exception& ex)
    {
        TryDeleteFile(tempPath);
        throw DOPluginException("Download hit an exception: %s", ex.what());
    }
    catch (...)
    {
        TryDeleteFile(tempPath);
        throw;
    }

    const HashResult hashResult = FileHashes(destFilePath);
    DownloadResult res;
    res.fileSizeBytes = hashResult.fileSizeBytes;
    res.hashes = std::vector<std::string>
    {
        "MD5-Hash", hashResult.md5Digest,
        "MD5Sum-Hash", hashResult.md5SumDigest,
        "SHA1-Hash", hashResult.sha1Digest,
        "SHA256-Hash", hashResult.sha256Digest,
        "SHA512-Hash", hashResult.sha512Digest
    };
    return res;
}

void WorkUriAcquire(const std::string& url, const std::string& filePath)
{
    try
    {
        if (!strutil::StartsWith(url, "http://") && !strutil::StartsWith(url, "https://"))
        {
            throw DOPluginException("Unsupported protocol: %s", url.data());
        }

        DownloadResult result = SimpleDownload(url, filePath);
        LogDebug("Download complete: %s to %s", url.data(), filePath.data());
        SendUriDone(url, result.fileSizeBytes, filePath, result.hashes);
        return;
    }
    catch (const DOPluginException& doex)
    {
        LogError("DO exception in download: %s", doex.what());
    }
    catch (const std::exception& ex)
    {
        LogError("C++ exception in download: %s", ex.what());
    }

    SendUriFailure(url, filePath);
}

void WorkLoop()
{
    // Pipeline:true indicates to apt that we support HTTP/1.1 (via libcurl).
    // Single-Instance:false allows apt to invoke multiple copies of us in parallel, thus speeding up operations.
    SendAPTMessage(APTMsgCodes::Capabilities,
        { "Version", msdoutil::SimpleVersion(), "Single-Instance", "false", "Send-Config", "false", "Pipeline", "true" });

    while (true)
    {
        APTCommand command = ReceiveAPTMessage();
        if (command.eof)
        {
            LogDebug("Received input EOF");
            break;
        }

        if (command.code == "600")
        {
            LogDebug("-- Command: Download --");
            const std::string url = command.headers["URI"];
            const std::string filePath = command.headers["Filename"];
            WorkUriAcquire(url, filePath);
        }
        else if (command.code == "601")
        {
            LogDebug("-- Command: Configuration --\n%s", strutil::MapToString(command.headers).c_str());
        }
        else
        {
            LogDebug("-- Command: Unsupported: %s %s --\n%s", command.code.c_str(), command.codeDescription.c_str(),
                strutil::MapToString(command.headers).c_str());
        }
    }
}

int main(int argc, char** argv)
{
    int exitCode = 0;
    try
    {
        if (msdoutil::OutputVersionIfNeeded(argc, argv))
        {
            return 0;
        }

        LogDebug("Start up with %d args, %s.", argc, msdoutil::ComponentVersion().c_str());

#ifdef DEBUG
        if (argc == 2)
        {
            if (strcmp(argv[1], "--dev-test") == 0)
            {
                WorkUriAcquire("http://archive.ubuntu.com/ubuntu/pool/main/g/gcc-4.7/cpp-4.7_4.7.4-3ubuntu12_i386.deb", "/tmp/gcc4.7.deb");
                WorkUriAcquire("cdp://repos/vscode2/dists/stable/InRelease", "/tmp/InRelease.dotmp");
                WorkUriAcquire("http://repos/vscode2/pool/main/c/code/code_1.27.2-1536736588_amd64.deb", "/tmp/code_1.27.2-1536736588_amd64.deb.dotmp");
                WorkUriAcquire("https://packages.microsoft.com/repos/vscode/pool/main/c/code/code_1.42.1-1581432938_amd64.deb", "/tmp/code_1.42.1-1581432938_amd64.deb");
                return 0;
            }

            printf("Unknown option: %s\n", argv[1]);
            return 1;
        }
#endif // DEBUG

        WorkLoop();
    }
    catch (const DOPluginException& doex)
    {
        LogError("DO exception: %s", doex.what());
        exitCode = 2;
    }
    catch (const std::exception& ex)
    {
        LogError("C++ exception: %s", ex.what());
        exitCode = 3;
    }
    catch (...)
    {
        LogError("Unknown exception");
        exitCode = 4;
    }

    LogDebug("Exit with %d\n", exitCode);
    return exitCode;
}
