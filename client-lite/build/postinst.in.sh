#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

do_group_name=@do_group@
do_user_name=@do_user@
config_path=@docs_svc_config_dir_path@
log_path=@docs_svc_log_dir_path@
svc_name=@docs_svc_name@
svc_config_path=@docs_systemd_cfg_path@
svc_bin_path=@docs_svc_bin_path@

# Exit early to fail the install if any command here fails
set -e

echo "Running post-install script for $svc_name"

if [ ! -f $svc_bin_path ]; then echo "docs binary cannot be found"; exit 1; fi

if ! getent group $do_group_name > /dev/null; then
    addgroup --system $do_group_name
fi

if ! getent passwd $do_user_name > /dev/null; then
    adduser --system $do_user_name --ingroup $do_group_name --shell /bin/false
fi

# Add each admin user to the do group - for systems installed before Ubuntu 12.04 LTS (Precise Pangolin)
# Use sed to parse the user name from a string in the form group:passwd:gid:member
for u in $(getent group admin | sed -e "s/^.*://" -e "s/,/ /g"); do
    adduser "$u" $do_group_name > /dev/null || true
done

# Add each sudo user to the do group
# Use sed to parse the user name from a string in the form group:passwd:gid:member
for u in $(getent group sudo | sed -e "s/^.*://" -e "s/,/ /g"); do
    adduser "$u" $do_group_name > /dev/null || true
done

configure_dir() {
    local dir_path="$1"
    echo "Configuring dir: $dir_path"
    if [ ! -d $dir_path ]; then
        mkdir $dir_path
    fi
    chgrp -R $do_group_name $dir_path
    chown $do_user_name $dir_path
}

configure_dir "$config_path"
# group needs write permission to config path (SDK writing configs)
chmod g+w $config_path
configure_dir "$log_path"

# See https://www.freedesktop.org/software/systemd/man/systemd.directives.html
echo "Installing $svc_name"
cat > ${svc_config_path} << EOF
[Unit]
Description=$svc_name: Performs content delivery optimization tasks

[Service]
ExecStart=$svc_bin_path
Restart=on-failure

[Install]
WantedBy=multi-user.target
EOF

echo "Service conf stored at: $svc_config_path"
echo "Service bin located at: $svc_bin_path"
echo "Reloading systemd daemon list and enabling $svc_name"
systemctl daemon-reload
systemctl enable $svc_name
systemctl stop $svc_name > /dev/null # stop if already running
systemctl start $svc_name
echo "Done!"
