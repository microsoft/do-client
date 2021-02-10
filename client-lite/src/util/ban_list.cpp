#include "do_common.h"
#include "ban_list.h"

void CBanList::Report(const std::string& name, std::chrono::milliseconds banInterval)
{
    _banList[name] = std::chrono::system_clock::now() + banInterval;
    DoLogInfo("%s banned for %lld ms", name.data(), static_cast<INT64>(banInterval.count()));
}

bool CBanList::IsBanned(const std::string& name)
{
    auto it = _banList.find(name);
    if (it == _banList.end())
    {
        return false;
    }

    const auto unbanTime = it->second;
    const auto now = std::chrono::system_clock::now();
    if (unbanTime > now)
    {
        const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(unbanTime - now);
        DoLogVerbose("%s will be unbanned after %lld ms", it->first.data(), static_cast<INT64>(diff.count()));
        return true;
    }

    _banList.erase(it);
    DoLogVerbose("%s removed from ban list", name.data());
    return false;
}
