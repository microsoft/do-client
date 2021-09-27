# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# NOTE: This script is a sample, partly created to detail all the dependencies required for building do-client components on windows

.\install-vcpkg-deps.ps1 $env:temp\deliveryoptimization_tools

# This step will require manual installation via .msi GUI
$ProgressPreference = 'SilentlyContinue'
Invoke-WebRequest -Uri https://github.com/Kitware/CMake/releases/download/v3.20.0/cmake-3.20.0-windows-x86_64.msi -OutFile "cmake-3.20.0-windows-x86_64.msi"
.\cmake-3.20.0-windows-x86_64.msi

$ProgressPreference = 'SilentlyContinue'
Invoke-WebRequest -Uri "https://www.python.org/ftp/python/3.7.0/python-3.7.0.exe" -OutFile "python-3.7.0.exe"
.\python-3.7.0.exe /quiet InstallAllUsers=1 PrependPath=1 Include_test=0

Write-Host "Finished bootstrapping"
