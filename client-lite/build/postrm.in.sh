#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

do_group_name=@do_group@
do_user_name=@do_user@
config_path=@docs_svc_config_dir_path@
log_path=@docs_svc_log_dir_path@
run_path=@docs_svc_run_dir_path@
svc_name=@docs_svc_name@
svc_config_path=@docs_systemd_cfg_path@

echo "**** Running post-removal script for $svc_name, args: $1 $2 ****"

do_remove_daemon() {
    echo "Removing systemd unit file: $svc_config_path"
    rm $svc_config_path

    echo "Reloading daemons"
    systemctl daemon-reload
}

do_remove_dirs() {
    echo "Removing log directory: $log_path"
    rm -rf $log_path

    echo "Removing run directory: $run_path"
    rm -rf $run_path
}

do_remove_user_and_group() {
    echo "Removing user: ${do_user_name}"
    userdel ${do_user_name}

    echo "Removing group: ${do_group_name}"
    groupdel ${do_group_name}
}

do_remove_tasks() {
    do_remove_daemon
    do_remove_dirs
    do_remove_user_and_group
}

case "$1" in
    remove)
        do_remove_tasks
    ;;

    purge)
        do_remove_tasks

        echo "Removing config directory: $config_path"
        rm -rf $config_path
    ;;

    # upgrade: Removing user/group/directories during an upgrade is a mistake that can impact group membership, historical logs, etc.
    # abort-install, abort-upgrade: This project does not use any package install path that requires handling these cases.
    upgrade|abort-install|abort-upgrade)
    ;;
esac

echo "**** Done! ****"
