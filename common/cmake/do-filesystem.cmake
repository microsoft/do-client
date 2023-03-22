
# The C++17 std::filesystem support is implemented in std::experimental::filesystem prior to GCC 9.
# However, and this is so messed up, only the experimental version requires linking to stdc++fs library.
# There is no built-in support in cmake to handle this difference.
# So this function attempts to determine which version is available using the check_cxx_source_compiles() feature.
# See also: Portable linking for C++17 std::filesystem (https://gitlab.kitware.com/cmake/cmake/-/issues/17834)
function (try_set_filesystem_lib)

    if (DO_PLATFORM_WINDOWS)
        # std::filesystem not required for Windows builds
        return ()
    endif ()

    if (CXX_FILESYSTEM_LIBS)
        # Already set, no need to redo
        return ()
    endif ()

    set(FS_TESTCODE
        "
        #if defined(CXX17_FILESYSTEM) || defined (CXX17_FILESYSTEM_LIBFS)
        #include <filesystem>
        #elif defined(CXX11_EXP_FILESYSTEM) || defined (CXX11_EXP_FILESYSTEM_LIBFS)
        #include <experimental/filesystem>
        namespace std {
            namespace filesystem {
                using experimental::filesystem::is_regular_file;
            }
        }
        #endif
        int main(void)
        {
            return std::filesystem::is_regular_file(\"/\") ? 0 : 1;
        }
        "
    )

    include (CMakePushCheckState)
    include (CheckCXXSourceCompiles)

    # Note that check_cxx_source_compiles automatically adds the result variable itself as a preprocessor symbol.

    cmake_push_check_state(RESET)
    check_cxx_source_compiles("${FS_TESTCODE}" CXX17_FILESYSTEM)
    if (NOT CXX17_FILESYSTEM)
        set(CMAKE_REQUIRED_LIBRARIES stdc++fs)
        check_cxx_source_compiles("${FS_TESTCODE}" CXX17_FILESYSTEM_LIBFS)
        cmake_reset_check_state()
    endif ()

    if (NOT CXX17_FILESYSTEM AND NOT CXX17_FILESYSTEM_LIBFS)
        check_cxx_source_compiles("${FS_TESTCODE}" CXX11_EXP_FILESYSTEM)
        if (NOT CXX11_EXP_FILESYSTEM)
            set(CMAKE_REQUIRED_LIBRARIES stdc++fs)
            check_cxx_source_compiles("${FS_TESTCODE}" CXX11_EXP_FILESYSTEM_LIBFS)
        endif ()
    endif ()
    cmake_pop_check_state()

    if (CXX17_FILESYSTEM_LIBFS OR CXX11_EXP_FILESYSTEM_LIBFS)
        set(CXX_FILESYSTEM_LIBS stdc++fs PARENT_SCOPE)
    endif ()

    unset(FS_TESTCODE)

endfunction ()
