# NOTE: This script is a sample, partly created to detail all the dependencies required for building do-client components on windows

$vcpkgdir=$args[0] # path to install vcpkgdir, i.e. C:\users\user\appdata\local\temp

Write-Host "Installing vcpkg to $vcpkgdir"

# You can also use the submoduled vcpkg directory within this project which comes with all necessary binaries pre-built
git clone https://github.com/microsoft/vcpkg $vcpkgdir\vcpkg

cd $vcpkgdir\vcpkg
.\bootstrap-vcpkg.bat

.\vcpkg install gtest:x64-windows boost:x64-windows gsl:x64-windows

