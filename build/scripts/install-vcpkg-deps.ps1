$vcpkgdir=$args[0] # path to install vcpkgdir, i.e. C:\users\user\appdata\local\temp

Write-Host "Installing vcpkg to $vcpkgdir"

# You can also use the submoduled vcpkg directory within this project which comes with all necessary binaries pre-built
git clone https://github.com/microsoft/vcpkg $vcpkgdir\vcpkg

cd $vcpkgdir\vcpkg
git checkout 2021.05.12
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
.\vcpkg update
.\vcpkg install gtest:x64-windows
.\vcpkg install boost-filesystem:x64-windows
.\vcpkg install boost-program-options:x64-windows

