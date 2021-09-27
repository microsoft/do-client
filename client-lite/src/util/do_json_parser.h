// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <chrono>
#include <string>

#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>

class JsonParser
{
public:
    static std::chrono::seconds RefreshInterval;

    JsonParser(const std::string& jsonFilePath, bool alwaysCreateFile = false);

    void Refresh();

    template<typename T>
    boost::optional<T> Get(const std::string& key)
    {
        _TryRefresh();
        boost::optional<T> value;
        try
        {
            value = _tree.get<T>(key);
        }
        catch (...)
        {
        }
        return value;
    }

private:
    void _TryRefresh(bool force = false);

    const std::string _jsonFilePath;
    boost::property_tree::ptree _tree;
    std::chrono::steady_clock::time_point _nextRefreshTime{};

};