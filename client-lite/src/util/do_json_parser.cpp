#include "do_common.h"
#include "do_json_parser.h"

#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>

std::chrono::seconds JsonParser::RefreshInterval = std::chrono::seconds(60);

// Stores file path provided. Loads from the file later when a value is queried for.
JsonParser::JsonParser(const std::string& jsonFilePath, bool alwaysCreateFile) :
    _jsonFilePath(jsonFilePath)
{
    if (!boost::filesystem::exists(_jsonFilePath) && alwaysCreateFile)
    {
        DoLogInfo("json file not found at %s, creating file", _jsonFilePath.data());
        boost::property_tree::ptree json;
        boost::property_tree::write_json(_jsonFilePath, json);
    }
}

void JsonParser::Refresh()
{
    _TryRefresh(true);
}

void JsonParser::_TryRefresh(bool force)
{
    if (std::chrono::steady_clock::now() < _nextRefreshTime && !force)
    {
        return;
    }

    if (boost::filesystem::exists(_jsonFilePath))
    {
        try
        {
            boost::property_tree::read_json(_jsonFilePath, _tree);
            DoLogInfo("Read json config file %s", _jsonFilePath.data());
        }
        catch (const std::exception& ex)
        {
            DoLogWarning("Could not read json config file %s: %s", _jsonFilePath.data(), ex.what());
        }
        catch (...)
        {
            DoLogWarning("Caught unexpected exception when reading json config file %s", _jsonFilePath.data());
        }
    }
    else
    {
        DoLogVerbose("json file not found at %s", _jsonFilePath.data());
        _tree.clear();
    }

    _nextRefreshTime = std::chrono::steady_clock::now() + RefreshInterval;
}
