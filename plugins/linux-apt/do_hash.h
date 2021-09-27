// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <string>

struct HashResult
{
    size_t fileSizeBytes;
    std::string md5Digest;
    std::string md5SumDigest;
    std::string sha1Digest;
    std::string sha256Digest;
    std::string sha512Digest;
};

HashResult FileHashes(const std::string& filePath);
