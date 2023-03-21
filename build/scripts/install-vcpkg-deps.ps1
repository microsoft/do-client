# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

if (-not (Test-Path "$env:VCPKG_INSTALLATION_ROOT\vcpkg.exe"))
{
    Write-Host "vcpkg not found in $env:VCPKG_INSTALLATION_ROOT"
    exit 1
}

cd $env:VCPKG_INSTALLATION_ROOT
git checkout 2021.05.12
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
.\vcpkg update
.\vcpkg install gtest:x64-windows
.\vcpkg install boost-program-options:x64-windows
