#!/bin/bash

# Exit early to fail the install if any command here fails
set -e

echo "Running post-install script for @target_bin@"

echo "Configuring DO plugin as the http(s) method"
if [ ! -f @dopapt_bin_path@ ]; then
    echo "Plugin binary cannot be found at @dopapt_bin_path@";
    return 1
fi

if [ ! -d @dopapt_cache_path@ ]; then
    echo "Creating @dopapt_cache_path@"
    mkdir -p @dopapt_cache_path@
fi

if getent passwd @do_user_group@ > /dev/null; then
    echo "Configuring @do_user_group@ for write access into plugin cache"
    chgrp @do_user_group@ @dopapt_cache_path@
    chmod g+w @dopapt_cache_path@
else
    echo "DO user not found. Not changing permissions for @dopapt_cache_path@."
fi

# Backup
if [ -f @apt_methods_root@/http ]; then
    mv @apt_methods_root@/http @apt_methods_root@/http-backup
    # https is usually a symlink to http so no need to back it up
    ls -l @apt_methods_root@/http-backup
else
    echo "http method not found in @apt_methods_root@. Nothing to backup."
fi

echo "Creating symlink @apt_methods_root@/http -> @dopapt_bin_path@"
ln -s @dopapt_bin_path@ @apt_methods_root@/http

echo "Configured http(s) method to use the DO plugin"
ls -l @apt_methods_root@/http

echo "Done!"
