// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

#include <boost/filesystem.hpp>

#include "do_common.h"
#include "do_persistence.h"

inline gid_t GetGroupIdByName(const char *name)
{
    struct group *grp = getgrnam(name); // don't free, see getgrnam() for details
    if (grp == nullptr)
    {
        THROW_HR_MSG(E_FAIL, "Failed to get gid from %s, errno: %d", name, errno);
    }
    return grp->gr_gid;
}

inline uid_t GetUserIdByName(const char *name)
{
    struct passwd *pwd = getpwnam(name); // don't free, see getpwnam() for details
    if (pwd == nullptr)
    {
        THROW_HR_MSG(E_FAIL, "Failed to get gid from %s, errno: %d", name, errno);
    }
    return pwd->pw_uid;
}

inline void SetDOPathPermissions(const std::string& path, mode_t mode)
{
    uid_t userid = GetUserIdByName("do");
    gid_t groupid = GetGroupIdByName("do");

    int err = chown(path.c_str(), userid, groupid);
    if (err < 0)
    {
        DoLogWarning("Failed to set DO ownership of path %s, error code: %d", path.c_str(), err);
    }

    err = chmod(path.c_str(), mode);
    if (err  < 0)
    {
        DoLogWarning("Failed to modify permissions for path %s, error code: %d", path.c_str(), err);
    }
}

inline void InitializePath(const std::string& path, mode_t mode = 0) try
{
    boost::filesystem::path dirPath(path);
    if (!boost::filesystem::exists(dirPath))
    {
        DoLogInfo("Creating directory at %s", path.c_str());
        boost::filesystem::create_directory(dirPath);

        if (mode != 0)
        {
#ifndef DO_BUILD_FOR_SNAP
            SetDOPathPermissions(path, mode);
#endif
        }
    }
} CATCH_LOG()

inline void InitializeDOPaths()
{
    // Config directory may have caller setting configs - therefore it should have group write permissions bit S_IWGRP set
    InitializePath(docli::GetConfigDirectory(), S_IRWXU | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
    // No external process or linux user will be writing to the log directory, so no need for S_IWGRP
    InitializePath(docli::GetLogDirectory(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    InitializePath(docli::GetRuntimeDirectory());
}

inline void DropPermissions()
{
    uid_t userid = GetUserIdByName("do");

    // process is running as root, drop privileges
    if (getuid() == 0)
    {
        gid_t groupid = GetGroupIdByName("do");
        if (initgroups("do", groupid) != 0)
        {
            THROW_HR_MSG(E_FAIL, "initgroups: Unable to initialize supplementary group access list: errno: %d", errno);
        }
        if (setgid(groupid) != 0)
        {
            THROW_HR_MSG(E_FAIL, "setgid: Unable to drop group privileges: %u, errno: %d", groupid, errno);
        }
        if (setuid(userid) != 0)
        {
            THROW_HR_MSG(E_FAIL, "setuid: Unable to drop user privileges: %u, errno: %d", userid, errno);
        }
    }
    else
    {
#ifndef DO_DEV_DEBUG
        // Unexpected to be running as non-root when not in devdebug mode
        THROW_HR_MSG(E_FAIL, "Attempting to drop permissions while not Root, uid: %u", getuid());
#endif
    }
}
