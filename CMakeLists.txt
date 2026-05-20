cmake_minimum_required(VERSION 3.14)

project(iso15118
    VERSION 0.8.0
    DESCRIPTION "iso15118 library suite"
	LANGUAGES CXX C
)

find_package(everest-cmake 0.5
    PATHS ../everest-cmake
    NO_DEFAULT_PATH
)
find_package(everest-cmake 0.5)

find_package(OpenSSL 3 REQUIRED)

if (NOT everest-cmake_FOUND)
    message(STATUS "Retrieving everest-cmake using FetchContent")
    include(FetchContent)
    FetchContent_Declare(
        everest-cmake
        GIT_REPOSITORY https://github.com/EVerest/everest-cmake.git
        GIT_TAG        v0.5.3
    )
    FetchContent_MakeAvailable(everest-cmake)
    set(everest-cmake_DIR "${everest-cmake_SOURCE_DIR}")
    include("${everest-cmake_SOURCE_DIR}/everest-cmake-config.cmake")
endif()

# options
option(${PROJECT_NAME}_BUILD_TESTING "Build unit tests, used if included as dependency" OFF)
option(BUILD_TESTING "Build unit tests, used if standalone project" OFF)
option(DISABLE_EDM "Disable use of the EVerest dependency manager" OFF)
option(OPT_AUTODOWNLOAD_ISO20_SCHEMAS "\
Automatically download ISO15118-20 schemas.  Note: by setting this option to \
true and hence downloading the schema files, YOU accept the ISO Customer \
Licence Agreement (“Licence Agreement”), clauses 1. ISO’s Copyright, \
7. Termination, 8. Limitations, and 9. Governing Law." OFF)

option(ISO15118_INSTALL "Enable install target" ${EVC_MAIN_PROJECT})
set(ISO15118_LIBCBV2G_GIT_REPOSITORY "https://github.com/mingshi2807/libcbv2g.git"
    CACHE STRING "libcbv2g Git repository used by the FetchContent fallback")
set(ISO15118_LIBCBV2G_GIT_TAG "a70b7c8a3209429fae4c956dc2fa7c79ada99526"
    CACHE STRING "libcbv2g Git tag or commit used by the FetchContent fallback")

# list of compile options
set(ISO15118_COMPILE_OPTIONS_WARNING "-Wall;-Wextra;-Wno-unused-function;-Werror" CACHE STRING "A list of compile options used")
message(STATUS "Building libiso15118 with the following compile options: ${ISO15118_COMPILE_OPTIONS_WARNING}")

if((${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME} OR ${PROJECT_NAME}_BUILD_TESTING) AND BUILD_TESTING)
    set(ISO15118_BUILD_TESTING ON)
endif()
option(ISO15118_BUILD_EXAMPLES "Build example applications" ${ISO15118_BUILD_TESTING})

# dependencies
if (NOT DISABLE_EDM)
    find_program(EDM_EXECUTABLE edm)
    if (NOT EDM_EXECUTABLE)
        message(STATUS "EVerest dependency manager not found; disabling EDM support and fetching dependencies directly")
        set(DISABLE_EDM ON)
    endif()
endif()

if (NOT DISABLE_EDM)
    evc_setup_edm()
    # In EDM mode, we can't install exports (because the dependencies usually do not install their exports)
    set(ISO15118_INSTALL OFF)
else()
    include(FetchContent)

    find_package(cbv2g QUIET)
    if (NOT cbv2g_FOUND)
        message(STATUS "Retrieving libcbv2g using FetchContent from ${ISO15118_LIBCBV2G_GIT_REPOSITORY} at ${ISO15118_LIBCBV2G_GIT_TAG}")
        # Keep install exports disabled when cbv2g is embedded through
        # FetchContent; its targets are not part of this package export set.
        set(ISO15118_INSTALL OFF)
        FetchContent_Declare(
            libcbv2g
            GIT_REPOSITORY ${ISO15118_LIBCBV2G_GIT_REPOSITORY}
            GIT_TAG        ${ISO15118_LIBCBV2G_GIT_TAG}
        )
        FetchContent_MakeAvailable(libcbv2g)
    endif()
endif()

if (ISO15118_BUILD_TESTING)
    include(FetchContent)
    find_package(Catch2 3 CONFIG QUIET)
    if (NOT Catch2_FOUND)
        message(STATUS "Retrieving Catch2 using FetchContent")
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG        v3.10.0
        )
        FetchContent_MakeAvailable(Catch2)
    endif()
endif()

add_subdirectory(input)
add_subdirectory(src)

option(ISO15118_BUILD_C_API "Build C wrapper for FFI interop" ON)
if(ISO15118_BUILD_C_API)
    add_subdirectory(api/c)
endif()

if (ISO15118_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (ISO15118_BUILD_TESTING)
    include(CTest)
    add_subdirectory(test)
endif()

if (ISO15118_INSTALL)
    install(
        TARGETS
            iso15118
        EXPORT iso15118-targets
    )

    install(
        DIRECTORY include/
        TYPE INCLUDE
        PATTERN "detail" EXCLUDE
    )

    evc_setup_package(
        NAME iso15118
        EXPORT iso15118-targets
        NAMESPACE iso15118
    )
endif()