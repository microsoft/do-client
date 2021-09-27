// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_hash.h"

#include <fstream>
#include <vector>

#include <openssl/md5.h>
#include <openssl/sha.h>

#include "do_plugin_exception.h"
#include "do_log.h"

HashResult FileHashes(const std::string& filePath)
{
    std::ifstream input(filePath, std::ios_base::in | std::ios_base::binary);
    if (input.bad())
    {
        throw DOPluginException("Failed to open file %s", filePath.data());
    }

    // log file info
    input.seekg(0, input.end);
    auto fileSizeBytes = static_cast<size_t>(input.tellg());
    input.seekg(0, input.beg);

    LogDebug("Hashing file (size = %lld bytes) : %s", fileSizeBytes, filePath.data());

    MD5_CTX stateMD5;
    SHA_CTX stateSHA1;
    SHA256_CTX stateSHA256;
    SHA512_CTX stateSHA512;
    MD5_Init(&stateMD5);
    SHA1_Init(&stateSHA1);
    SHA256_Init(&stateSHA256);
    SHA512_Init(&stateSHA512);

    std::vector<char> buffer(4096);
    while (true)
    {
        input.read(buffer.data(), buffer.size());
        if (input.bad())
        {
            throw DOPluginException("Failed to read file %s", filePath.data());
        }

        auto bytesRead = input.gcount();
        if (bytesRead > 0)
        {
            MD5_Update(&stateMD5, buffer.data(), bytesRead);
            SHA1_Update(&stateSHA1, buffer.data(), bytesRead);
            SHA256_Update(&stateSHA256, buffer.data(), bytesRead);
            SHA512_Update(&stateSHA512, buffer.data(), bytesRead);
        }

        if (input.eof())
        {
            break;
        }

        if (input.fail())
        {
            throw DOPluginException("Failed to read file %s, bytesRead now: %lld", filePath.data(), bytesRead);
        }
    }

    unsigned char md5sum[MD5_DIGEST_LENGTH];
    MD5_Final(md5sum, &stateMD5);

    unsigned char sha1sum[SHA_DIGEST_LENGTH];
    SHA1_Final(sha1sum, &stateSHA1);

    unsigned char sha256sum[SHA256_DIGEST_LENGTH];
    SHA256_Final(sha256sum, &stateSHA256);

    unsigned char sha512sum[SHA512_DIGEST_LENGTH];
    SHA512_Final(sha512sum, &stateSHA512);

    auto md5Str = strutil::HexEncode(md5sum, sizeof(md5sum));
    auto sha1Str = strutil::HexEncode(sha1sum, sizeof(sha1sum));
    auto sha256Str = strutil::HexEncode(sha256sum, sizeof(sha256sum));
    auto sha512Str = strutil::HexEncode(sha512sum, sizeof(sha512sum));

    HashResult res;
    res.fileSizeBytes = fileSizeBytes;
    res.md5Digest = md5Str;
    res.md5SumDigest = md5Str;
    res.sha1Digest = sha1Str;
    res.sha256Digest = sha256Str;
    res.sha512Digest = sha512Str;
    return res;
}
