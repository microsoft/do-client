"""Build script for local and azure pipeline builds."""

import datetime
import os, sys
import time
import timeit
import subprocess
import shutil
import argparse
import warnings
import tempfile
import fnmatch
from pathlib import Path
from tempfile import gettempdir

#region Globals

VERBOSE = True
DOCLIENT_SUBPROJECT_BUILD_MAP = {
    "sdk" : "-DDO_INCLUDE_SDK=TRUE",
    "agent" : "-DDO_INCLUDE_AGENT=TRUE",
    "plugin-apt" : "-DDO_INCLUDE_PLUGINS=TRUE"
}
#endregion

def main():
    """The main entry point."""
    # Set the global vars for the script
    global VERBOSE
    global DOCLIENT_SUBPROJECT_BUILD_MAP

    build = create_build_runner()
    build.run()

# region ArgParser

class ArgParserBase(object):
    """Base class for specific platform arg parser

    ArgParser classes will inherit from this class
    and will parse args specific to that build.

    """

    def __init__(self):
        self.parser = argparse.ArgumentParser(description='Builds the DeliveryOptimization client components')
        self.parser.add_argument(
            '--project', dest='project', type=str, required=True,
            help='The cmake subproject to build. e.g. {}'.format(list(DOCLIENT_SUBPROJECT_BUILD_MAP.keys()))
        )
        self.parser.add_argument(
            '--operation', dest='operation', type=str,
            help='The operation to perform. e.g. generate/build/cleanonly. Default is generate+build'
        )
        self.parser.add_argument(
            '--generator', dest='generator', type=str,
            help='The CMake generator to use. e.g. Ninja, Unix Makefiles'
        )
        self.parser.add_argument(
            '--config', dest='config', type=str,
            help='The target configuration. e.g. debug or release'
        )
        self.parser.add_argument(
            '--cmaketarget', dest='cmaketarget', type=str,
            help='The cmake target within each subproject to build. e.g. within sdk: deliveryoptimization-sdk-tests'
        )
        self.parser.add_argument(
            '--clean', dest='clean', action='store_true',
            help='Remove built binaries before re-building them'
        )
        self.parser.add_argument(
            '--skip-tests', dest='skip_tests', action='store_true',
            help='Skip adding and building test code'
        )
        self.parser.add_argument(
            '--static-analysis', dest='static_analysis', action='store_true',
            help='Run static analysis tools (cpplint)'
        )
        self.parser.add_argument(
            '--build-directory', dest='build_directory', type=str,
            help='Override default build output directory'
        )

    def parse(self):
        return self.parser.parse_args()

class LinuxArgParser(ArgParserBase):
    def __init__(self):
        super().__init__()
        self.parser.add_argument(
            '--package-for', dest='package_type', type=str,
            help='Supply package type. e.g. deb, or rpm'
        )

        '''Agent only'''
        self.parser.add_argument(
            '--no-proxy-support', dest='no_proxy_support', action='store_true',
            help='Enable building deliveryoptimization-agent without support for proxy handling'
        )

class WindowsArgParser(ArgParserBase):
    def __init__(self):
        super().__init__()
        raise ValueError("Windows builds not supported yet")

class MacArgPaser():
    def __init__(self):
        super().__init__()
        raise ValueError("Mac builds not supported yet")


# endregion ArgParser

#region BuildRunner classes

class BuildRunnerBase(object):
    """Base class for specific platform builds.

    BuildRunner classes will inherit from this class
    and will implement/override/add additional functionality
    for that specific build.

    Args:
        script_args (namespace):
            The arguments passed to the script parsed by argparse.
    """

    def __init__(self, script_args):
        super().__init__()
        self.timeToClean = 0
        self.timeToGenerate = 0
        self.timeToBuild = 0
        self.operation_type = script_args.operation
        self.project_root_path = get_project_root_path()
        self.cmake_target = None
        self.project = None
        self.build_directory = None

        if (script_args.cmaketarget is None):
            self.cmake_target = "all"
        else:
            self.cmake_target = script_args.cmaketarget
        if (script_args.project and script_args.project.lower() in DOCLIENT_SUBPROJECT_BUILD_MAP.keys()):
            self.project = script_args.project.lower()
        else:
            raise ValueError('Project name must be within {}'.format(list(DOCLIENT_SUBPROJECT_BUILD_MAP.keys())))
        self.script_args = script_args
        self.is_clean_build = self.script_args.clean

        if self.script_args.config:
            self.config = self.script_args.config.lower()
        elif get_env_var('BUILD_CONFIGURATION'):
            self.config = get_env_var('BUILD_CONFIGURATION').lower()
        else:
            self.config = 'debug'

        if self.config not in ['debug', 'release', 'relwithdebinfo', 'minsizerel']:
            raise ValueError('Building configuration for {self.platform} is not supported.'.format(self.config, self.platform))
        if self.script_args.generator:
            self.generator = self.script_args.generator

        self.skip_tests = script_args.skip_tests

        self.source_path = self.project_root_path

        self.build_time = datetime.datetime.utcnow().strftime("%Y%m%d.%H%M%S")

        if self.script_args.build_directory:
            self.build_directory = self.script_args.build_directory

    @property
    def compiler(self):
        """The compiler to use.

        Should be overriden by subclass.

        Returns:
            The name of the compiler.
            e.g. clang, gnu, msvc
        """
        pass

    @property
    def flavor(self):
        """The unique flavor string for this build.

        Returns:
            The unique flavor string for this build.
            e.g. linux-debug
        """
        return '{}-{}'.format(self.platform, self.config)

    @property
    def generator(self):
        """The CMake generator for this build.

        Can be overriden by subclass.
        Tells CMake what type of build to generate.
        'Unix Makefiles' will generate a set of make files,
        'Visual Studio ...' will generate vs proj files,
        etc.

        Returns:
            CMake generator ID as string.
            None if default generator should be used (not recommended).
        """
        if is_running_on_linux():
            return "Ninja"
        return None

    @property
    def platform(self):
        """The target platform.

        Should be overriden by subclass.

        Returns:
            The target platform string.
            e.g. windows, linux
        """
        pass

    @property
    def build_path(self):
        """Path for the build."""
        return get_build_path(self.project, self.build_directory, self.flavor)

    def run(self):
        if self.cmake_target != None:
            """Executes the Build."""
            self.print_start_build_msg()

            if self.is_clean_build:
                self.clean()

            if self.operation_type:
                if self.operation_type.lower() == "generate":
                    self.generate()
                elif self.operation_type.lower() == "build":
                    self.build()
                elif self.operation_type.lower() == "cleanonly":
                    if not self.is_clean_build:
                        self.clean()
                else:
                    raise ValueError('Invalid operation specified: {}'.format(self.operation_type))
            else:
                self.generate()
                self.build()

            self.print_end_build_msg()
            self.print_times()

    def print_start_build_msg(self):
        """Prints a message at the start of Build.run.

        Can be overriden by subclass.
        Typically subclasses will call
        super().print_start_build_msg before adding their own
        print statements.
        """
        print('Starting Build for project: {}'.format(self.project))
        print('Target OS: {}'.format(self.platform.capitalize()))
        print('Flavor: {}'.format(self.flavor))
        print('Config: {}'.format(self.config))
        print('Subproject: {}'.format(self.project))
        print('CMake Target: {}'.format(self.cmake_target))
        print('CMake Generator: {}'.format(self.generator))
        print('Clean: {}'.format(self.is_clean_build))
        print('Source Path: {}'.format(self.source_path))
        print('Build Path: {}'.format(self.build_path))

    def print_end_build_msg(self):
        """Prints a message at the end of Build.run."""
        print('Build Complete')

    def print_times(self):
        print('Time to clean: {}'.format(self.timeToClean))
        print('Time to generate: {}'.format(self.timeToGenerate))
        print('Time to build: {}'.format(self.timeToBuild))

    def clean(self):
        """Deletes the output directory(s) for this Build."""
        build_path = self.build_path
        print('Purging: {}'.format(build_path))
        start_time = timeit.default_timer()
        if os.path.exists(build_path):
            shutil.rmtree(build_path)
        self.timeToClean = timeit.default_timer() - start_time

    def generate(self):
        """Executes the generate phase of the build."""

        # Only Windows versions of cmake have
        # -S <source dir> or -B <build dir> options.
        # To support cmake on all platforms,
        # we need to create and change to our build output dir.
        original_dir = os.getcwd()
        os.makedirs(self.build_path, exist_ok=True)
        os.chdir(self.build_path)
        generate_command = self.create_generate_command()
        start_time = timeit.default_timer()
        run_command(generate_command)
        self.timeToGenerate = timeit.default_timer() - start_time
        os.chdir(original_dir)

    def create_generate_command(self):
        """Creates the command to use in the generate phase.

        Subclasses can override this method,
        but most likely subclasses will want to
        override generate_options instead.

        Returns:
            The generate command as a list of strings.
        """
        return ['cmake', self.source_path] + self.generate_options

    @property
    def generate_options(self):
        """Additional options to use in generate.

        Can be overriden by subclass.
        Typically subclasses will call
        super().generate_options + ['--foo', 'My option value']
        to add their own options to the generate_command list.

        Returns:
            The list of additional generate options.
        """
        generate_options = []
        if self.generator:
            generate_options.extend(['-G', self.generator])

        if self.config == "debug":
            generate_options.extend(["-DCMAKE_BUILD_TYPE=Debug"])
        elif self.config == "relwithdebinfo":
            generate_options.extend(["-DCMAKE_BUILD_TYPE=RelWithDebInfo"])
        elif self.config == "minsizerel":
            generate_options.extend(["-DCMAKE_BUILD_TYPE=MinSizeRel"])
        else:
            generate_options.extend(["-DCMAKE_BUILD_TYPE=Release"])

        # All pipelines perform a clean build so timestamp will get refreshed
        # even though we can pass this only to the generate phase.
        generate_options.extend(['-DDO_BUILD_TIMESTAMP={}'.format(self.build_time)])

        if self.skip_tests:
            generate_options.extend(["-DDO_BUILD_TESTS=OFF"])

        if self.project:
            generate_options.extend([DOCLIENT_SUBPROJECT_BUILD_MAP[self.project]])

        return generate_options

    def build(self):
        """Executes the build phase of the build."""
        build_command = self.create_build_command()
        print('Executing: {}'.format(' '.join(build_command)))
        start_time = timeit.default_timer()
        run_command(build_command)
        self.timeToBuild = timeit.default_timer() - start_time

    def create_build_command(self):
        """Creates the command to use in the build phase.

        Subclasses can override this method,
        but most likely subclasses will want to
        override build_options instead.

        Returns:
            The build command as a list of strings.
        """
        return ['cmake', '--build', self.build_path] + self.build_options

    @property
    def build_options(self):
        """Additional options to use in build.

        Can be overriden by subclass.
        Typically subclasses will call
        super().build_options + ['--foo', 'My option value'].

        Returns:
            The list of additional build options.
        """
        return ["--target", self.cmake_target]

    def package(self):
        subprocess.call(['/bin/bash', '-c', 'cd {} && cpack .'.format(self.build_path)])

class LinuxBuildRunner(BuildRunnerBase):
    """Linux BuildRunner class."""

    def __init__(self, script_args):
        super().__init__(script_args)

        self.package_type = None
        if self.script_args.package_type:
            self.package_type = self.script_args.package_type.lower()

        self.static_analysis = self.script_args.static_analysis

    @property
    def platform(self):
        return 'linux'

    @property
    def generator(self):
        return 'Ninja'

    @property
    def generate_options(self):
        generate_options = super().generate_options

        if self.package_type:
            if self.package_type in ["deb", "debian"]:
                generate_options.extend(["-DDO_PACKAGE_TYPE=DEB"])
            elif self.package_type == "rpm":
                generate_options.extend(["-DDO_PACKAGE_TYPE=RPM"])
            else:
                raise ValueError('{} is not a supported package_type'.format(self.package_type))

        if self.static_analysis:
            generate_options.extend(["-DCMAKE_CXX_CPPLINT=cpplint"])

        return generate_options

    def run(self):
        super().run()
        if self.package_type:
            self.package()

    def package(self):
        subprocess.call(['/bin/bash', '-c', 'cd {} && cpack .'.format(self.build_path)])

class WindowsBuildRunner(BuildRunnerBase):
    """Windows BuildRunner class."""

    def __init__(self, script_args):
        super().__init__(script_args)
        # Windows generates the vcxproj for all as ALL_BUILD.vcxproj, so need to change the target here
        if (self.cmake_target == 'all'):
            self.cmake_target = 'ALL_BUILD'

        if self.script_args.arch:
            self.arch = self.script_args.arch.lower()
        elif get_env_var('BUILD_ARCHITECTURE'):
            self.arch = get_env_var('BUILD_ARCHITECTURE')
        else:
            self.arch = 'x64'

        if self.arch.startswith('arm'):
            raise ValueError("Windows Arm Builds are not supported right now.")

        self.vcpkg_root_path = get_env_var('BUILD_VCPKGDIR') if script_args.vcpkgdir is None else script_args.vcpkgdir

    @property
    def platform(self):
        return 'windows'

    @property
    def compiler(self):
        return 'msvc'

    @property
    def generator(self):
        # No need to specify architecture here as the default target platform name (architecture) is that of the host and is provided in the CMAKE_VS_PLATFORM_NAME_DEFAULT variable
        # https://cmake.org/cmake/help/latest/generator/Visual%20Studio%2016%202019.html
        return 'Visual Studio 16 2019'

    @property
    def generate_options(self):
        return super().generate_options + [
                f'-DCMAKE_TOOLCHAIN_FILE={get_vcpkg_toolchain_file_path(self.vcpkg_root_path)}',
                f'-DVCPKG_TARGET_TRIPLET={self._vcpkg_triplet}'
            ]

    @property
    def build_options(self):
        return super().build_options + ['--config', self.config]

    @property
    def _vcpkg_triplet(self):
        """The triplet string required by vcpkg."""
        # There is no static version of the arm or arm64 packages.
        # if self.arch.startswith('arm'):
        #     return f'{self.arch}-{self.platform}'
        # else:
        #     return f'{self.arch}-{self.platform}-static'

        # We don't use the 'static' vcpkg install yet.
        # Looks like vcpkg uses static libs by default.
        return f'{self.arch}-{self.platform}'

class MacBuildRunner(BuildRunnerBase):
    def __init__(self, script_args):
        super().__init__(script_args)
        self.vcpkg_root_path = get_env_var('BUILD_VCPKGDIR') if script_args.vcpkgdir is None else script_args.vcpkgdir

    @property
    def platform(self):
        return 'darwin'

    @property
    def compiler(self):
        return 'clang'

    @property
    def generator(self):
        return 'ninja'

    @property
    def generate_options(self):
        return super().generate_options + [
                f'-DCMAKE_TOOLCHAIN_FILE={get_vcpkg_toolchain_file_path(self.vcpkg_root_path)}',
                f'-DVCPKG_TARGET_TRIPLET={self._vcpkg_triplet}'
            ]

    @property
    def build_options(self):
        return super().build_options + ['--config', self.config]

    @property
    def _vcpkg_triplet(self):
        """The triplet string required by vcpkg."""
        # There is no static version of the arm or arm64 packages.
        # if self.arch.startswith('arm'):
        #     return f'{self.arch}-{self.platform}'
        # else:
        #     return f'{self.arch}-{self.platform}-static'

        # We don't use the 'static' vcpkg install yet.
        # Looks like vcpkg uses static libs by default.
        return f'{self.arch}-{self.platform}'

#endregion BuildRunner Classes

#region Util Functions

def create_build_runner():
    """Creates the appropriate subclass of BuildRunnerBase.

    Chooses the correct BuildRunner class for the target platform.
    Args:
        script_args (namespace):
            The arguments passed to the script parsed by argparse.

    Returns:
        The appropriate subclass of Build.
    """

    if is_running_on_linux():
        script_args = LinuxArgParser().parse()
        return LinuxBuildRunner(script_args)
    elif is_running_on_windows():
        script_args = WindowsArgParser().parse()
        return WindowsBuildRunner(script_args)
    elif is_running_on_osx():
        script_args = MacArgPaser().parse()
        return MacBuildRunner(script_args)

    raise ValueError('Target platform is either not supported or could not be deduced from build environment')

def get_os_name():
    """Gets the friendly OS name.

    This value can differ for local builds vs pipeline builds.

    Returns:
        The friendly version of the OS Name.
    """
    if get_env_var('AGENT_OS'):
        return get_env_var('AGENT_OS').lower()
    else:
        return sys.platform.lower()

def is_running_on_linux():
    """Indicates if this build is running on a Linux agent/machine

    Returns:
        True if the build is running on a Linux agent/machine.
        False otherwise.
    """
    return get_os_name().startswith('linux')

def is_running_on_windows():
    """Indicates if this build is running on a Windows agent/machine

    Returns:
        True if the build is running on a Windows agent/machine.
        False otherwise.
    """
    return get_os_name().startswith('win')

def is_running_on_osx():
    """Indicates if this build is running on a OsX agent/machine

    Returns:
        True if the build is running on a Osx agent/machine.
        False otherwise.
    """
    return get_os_name().startswith('darwin')

def get_project_root_path():
    """Gets the root path to our git repo.

    Note that this function may return a different value
    than what is expected after calling os.chdir.

    Returns:
        The root path to our git repo.
    """
    script_path = os.path.dirname(os.path.realpath(__file__))
    print('script_path={}'.format(script_path))
    return os.path.abspath(os.path.join(script_path, '..'))

def get_cmake_files_path(root_path=None):
    """Gets the path to custom cmake 'include' files for our build
    TODO(shishirb) unused method

    Args:
        root_path (str):
            The project root path.
            If None, uses get_project_root_path() instead.

    Returns:
        The path to our custom cmake 'include' files.
    """
    if root_path is None:
        root_path = get_project_root_path()
    return os.path.abspath(os.path.join(root_path, 'build', 'cmake'))

def get_vcpkg_toolchain_file_path(root_path):
    """Gets the path to the vcpkg cmake toolchain file.

    Args:
        root_path (str):
            The vcpkg root path.

    Returns:
        The path to the vcpkg cmake toolchain file.
    """
    return os.path.abspath(os.path.join(root_path, 'scripts', 'buildsystems', 'vcpkg.cmake'))

def get_build_path(project, build_directory=None, flavor=None):
    """Gets the default path to the build folder.

    Uses the 'flavor' property to construct the path if available.

    Args:
        flavor (str):
            The unique flavor string for the build.

    Returns:
        The default bin path.
    """
    if build_directory == None:
        build_directory = tempfile.gettempdir()
    build_path = os.path.join(build_directory, "build-deliveryoptimization-" + project, flavor)
    return build_path

def get_env_var(name):
    """Gets the environment variable value or None.

    Utility function to get an environment variable value
    given the name of the environment variable.
    Returns None if the environment variable is not set/present.

    Args:
        name (str):
            The name of the environment variable.

    Returns:
        The value of the environment variable with name.
        None if the environment variable is not set/present.
    """
    if name.upper() in os.environ:
        return os.environ[name.upper()]
    else:
        return None

def run_command(command):
    """Runs the given command.

    Args:
        command (list):
            The command to run in list form.

    Raises:
        subprocess.CalledProcessError
    """
    command_string = ' '.join(command)
    try:
        print('Running command {}.'.format(command_string))
        _check_call(command)
    except subprocess.CalledProcessError:
        print('Running {} failed. Rethrowing exception'.format(command_string))
        raise

def _check_call(command):
    """Wrapper around subprocess.check_call.

    Handles piping output in various cases:
    - Verbose logging turned on/off.

    Args:
        command (list):
            The command to run in list form.

    Raises:
        subprocess.CalledProcessError
    """
    # We pipe stderr to stdout because
    # some commands (like apt) write output to stderr,
    # but that output should not cause a failure
    # in the pipeline build job.
    global VERBOSE
    if VERBOSE:
        subprocess.check_call(
            command,
            stderr=subprocess.STDOUT
        )
    else:
        subprocess.check_call(
            command,
            stderr=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL
        )
#endregion Util Functions

if __name__ == "__main__":
    main()
