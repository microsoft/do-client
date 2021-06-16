#!/bin/bash

do_group_name=@do_group@
do_user_name=@do_user@
config_path=@docs_svc_config_dir_path@
log_path=@docs_svc_log_dir_path@
persistence_path=@docs_svc_persistence_dir_path@
run_path=@docs_svc_run_dir_path@
svc_name=@docs_svc_name@
svc_config_path=@docs_systemd_cfg_path@

echo "Running post-removal script for $svc_name"

systemctl reset-failed $svc_name # Remove ourselves from the failed services list. No-op if never failed earlier.

echo "Removing systemd unit file: $svc_config_path"
rm $svc_config_path

echo "Reloading daemons"
systemctl daemon-reload

echo "Removing working directory: $persistence_path"
rm -rf $persistence_path

echo "Removing log directory: $log_path"
rm -rf $log_path

echo "Removing config directory: $config_path"
rm -rf $config_path

echo "Removing run directory: $run_path"
rm -rf $run_path

echo "Removing user: ${do_user_name}"
userdel ${do_user_name}

echo "Removing group: ${do_group_name}"
groupdel ${do_group_name}

echo "Done!"
