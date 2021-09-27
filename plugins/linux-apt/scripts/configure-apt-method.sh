#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Use this script to configure/remove DO plugin as the http(s) method for APT
# Usage: <script.sh> <plugin_bin_path> [<-r|--remove>]

# Ensure that getopt starts from first option if ". <script.sh>" was used.
OPTIND=1

# Ensure we don't end the user's terminal session if invoked from source (".").
if [[ $0 != "${BASH_SOURCE[0]}" ]]; then
    ret=return
else
    ret=exit
fi

apt_methods_root=/usr/lib/apt/methods
dopapt_cache_path=/var/cache/deliveryoptimization-plugin-apt
do_user_group=do

print_help() {
    echo "Usage: <script.sh> [options...]"
    echo "-i, --install <plugin_bin_path>   Configure provided DO plugin as the http(s) method."
    echo "                                  Existing http(s) method will be backed up first."
    echo ""
    echo "-r, --remove                      Restore original http(s) method. The DO plugin binary is not removed."
    echo "-h, --help                        Show this help message."
}

backup_apt_methods() {
    if [ -f $apt_methods_root/http ]; then
        mv $apt_methods_root/http $apt_methods_root/http-backup || return
        # https is usually a symlink to http so no need to back it up
        ls -l $apt_methods_root/http-backup
    else
        echo "http method not found in $apt_methods_root. Nothing to backup."
    fi
}

remove_dopapt_cache_path() {
    if [ -d $dopapt_cache_path ]; then
        echo "Removing $dopapt_cache_path"
        rm -rf $dopapt_cache_path || return
    fi
}

create_dopapt_cache_path() {
    remove_dopapt_cache_path    # Attempt to remove a left over cache path

    if [ ! -d $dopapt_cache_path ]; then
        echo "Creating $dopapt_cache_path"
        mkdir -p $dopapt_cache_path || return
    fi

    if getent passwd $do_user_group > /dev/null; then
        echo "Configuring $do_user_group for write access into plugin cache"
        chgrp $do_user_group $dopapt_cache_path || return
        chmod g+w $dopapt_cache_path || return
        ls -l $dopapt_cache_path/..
    else
        echo "DO user not found. Not changing permissions for $dopapt_cache_path."
    fi
}

install_dopapt_method() {
    echo "Configuring DO plugin as the http(s) method"
    if [ ! -f $dopapt_bin_path ]; then
        echo "Plugin binary cannot be found at $dopapt_bin_path";
        return 1
    fi
    backup_apt_methods || return
    echo "Creating symlink $apt_methods_root/http -> $dopapt_bin_path"
    ln -s $dopapt_bin_path $apt_methods_root/http || return
}

restore_apt_methods() {
    echo "Restoring method"
    if [ -f $apt_methods_root/http-backup ]; then
        rm $apt_methods_root/http || return
        mv $apt_methods_root/http-backup $apt_methods_root/http || return
    else
        echo "Method backup not found. Not doing anything."
        return 1
    fi
}

# ---- main ----

if [[ $1 == "" ]]; then
    error "Must specify at least one option."
    $ret 1
fi

while [[ $1 != "" ]]; do
    case $1 in
    -r | --remove)
        restore_apt_methods || $ret
        echo "Restored original methods"
        ls -l $apt_methods_root
        remove_dopapt_cache_path || $ret
        $ret 0
        ;;
    -i | --install)
        shift
        dopapt_bin_path=$(realpath "$1") || $ret
        create_dopapt_cache_path || $ret
        install_dopapt_method || $ret
        echo "Configured http(s) method to use the DO plugin"
        ls -l $apt_methods_root
        ;;
    -h | --help)
        print_help
        $ret 0
        ;;
    *)
        error "Invalid argument: $*"
        $ret 1
        ;;
    esac
    shift
done

$ret 0
