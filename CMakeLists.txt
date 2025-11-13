cmake_minimum_required (VERSION 3.14)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define _WIN32_WINNT to be at least Windows 8.1, Update 1 (KBSPRING14)
# (Needed for advanced virtual memory management)
add_compile_definitions(_WIN32_WINNT=0x0603)
add_compile_definitions(WINBLUE_KBSPRING14)
if (MSVC)
    add_compile_options(/Zc:__cplusplus /std:c++20)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

project (libgeodesk)

# TODO: What should be the default?
set(BUILD_SHARED_LIBS OFF)

option(GEODESK_PYTHON "Build GeoDesk with Python support" OFF)
option(GEODESK_PYTHON_WHEELS "Enable Support for Python Wheels" OFF)
option(GEODESK_EXAMPLES "Build example applications" OFF)
option(GEODESK_MULTITHREADED "Allow multiple threads to use the same GOL" OFF)
option(GEODESK_LIBERO "Enable Libero storage engine" ON)
option(GEODESK_WITH_GEOS "Build GeoDesk with GEOS support" OFF)
option(GEODESK_WITH_OGR "Build GeoDesk with OGR support" OFF)

# Option to choose between static or shared library
# Only set the option if BUILD_SHARED_LIBS is not already defined
if (NOT DEFINED BUILD_SHARED_LIBS)
    option(BUILD_SHARED_LIBS "Build shared library" ON)
endif()

include(FetchContent)

FetchContent_Declare(gtl
    GIT_REPOSITORY https://github.com/greg7mdp/gtl.git
    GIT_TAG v1.2.0)
FetchContent_MakeAvailable(gtl)

set(PREVIOUS_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)
set(CATCH_BUILD_STATIC_LIBRARY ON)
set(GEODESK_LIBERO ON)
# Fetch Catch2 from its GitHub repository
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(Catch2)
set(BUILD_SHARED_LIBS ${PREVIOUS_BUILD_SHARED_LIBS})

# Set the CMAKE_CXX_FLAGS_RELWITHDEBINFO variable to remove the NDEBUG definition
# This approach ensures that assertions are enabled in RelWithDebInfo configuration
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -U NDEBUG")

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

# Create the library
add_library(geodesk ${SOURCE_FILES})
if(GEODESK_MULTITHREADED)
    message(STATUS "GeoDesk: Multi-threaded query access")
    target_compile_definitions(geodesk PUBLIC GEODESK_MULTITHREADED)
else()
    message(STATUS "GeoDesk: Single-threaded query access")
endif()
if(GEODESK_LIBERO)
    message(STATUS "GeoDesk: Libero storage engine enabled")
    target_compile_definitions(geodesk PUBLIC GEODESK_LIBERO)
else()
    message(STATUS "GeoDesk: Legacy BlobStore storage engine enabled")
endif()
target_compile_features(geodesk PUBLIC cxx_std_20)
set_target_properties(geodesk PROPERTIES CXX_VISIBILITY_PRESET hidden)
set(INCLUDES include src ${boost_crc_SOURCE_DIR}/include)
if(GEODESK_WITH_GEOS)
    message(STATUS "GeoDesk: Building with GEOS support.")
    target_compile_definitions(geodesk PUBLIC GEODESK_WITH_GEOS)
    if(NOT DEFINED GEOS_INCLUDE_PATHS)
        message(FATAL_ERROR "GeoDesk: GEOS_INCLUDE_PATHS must be set to the location of GEOS.")
    endif()
    message(STATUS "GEOS_INCLUDE_PATHS = ${GEOS_INCLUDE_PATHS}")
    list(APPEND INCLUDES ${GEOS_INCLUDE_PATHS})
    target_link_libraries(geodesk PUBLIC geos geos_c)
else()
    message(STATUS "GeoDesk: Building *without* GEOS support.")
endif()
if(GEODESK_WITH_OGR)
    message(STATUS "GeoDesk: Building with OGR support.")
    target_compile_definitions(geodesk PUBLIC GEODESK_WITH_OGR)
    if(NOT DEFINED GDAL_INCLUDE_PATHS)
        message(FATAL_ERROR "GeoDesk: GDAL_INCLUDE_PATHS must be set to the  location of GDAL.")
    endif()
    message(STATUS "GDAL_INCLUDE_PATHS = ${GDAL_INCLUDE_PATHS}")
    list(APPEND INCLUDES ${GDAL_INCLUDE_PATHS})
endif()
message(STATUS "GeoDesk: INCLUDES = ${INCLUDES}")

if(GEODESK_PYTHON)
    target_compile_definitions(geodesk PUBLIC GEODESK_PYTHON)
    # Use Python paths set by cibuildwheel if available
    if(DEFINED ENV{CIBW_PYTHON_INCLUDE} AND DEFINED ENV{CIBW_PYTHON_LIBRARIES})
        message(STATUS "GeoDesk: Using Python settings from CIBuildWheels")
        set(Python_INCLUDE_DIRS $ENV{CIBW_PYTHON_INCLUDE})
        set(Python_LIBRARIES $ENV{CIBW_PYTHON_LIBRARIES})
    else()
        # Fallback to standard CMake Python find
        #if(GEODESK_PYTHON_WHEELS)
        #    message(STATUS "GeoDesk: Building with Python support (PythonExtensions).")
        #    find_package(PythonExtensions REQUIRED)
        #else()
            message(STATUS "GeoDesk: Building with Python support (Development).")
            find_package(Python REQUIRED COMPONENTS Development.Module)
        #endif()
    endif()
    message(STATUS "Python_INCLUDE_DIRS: ${Python_INCLUDE_DIRS}")
    message(STATUS "Python_LIBRARIES: ${Python_LIBRARIES}")
    target_include_directories(geodesk PUBLIC ${Python_INCLUDE_DIRS})
    target_link_libraries(geodesk PUBLIC ${Python_LIBRARIES})
endif()

# Ensure correct export/import of symbols for shared libraries
if(BUILD_SHARED_LIBS)
    message(STATUS "GeoDesk: Building dynamic library")
    target_compile_definitions(geodesk PUBLIC GEODESK_SHARED)
    target_compile_definitions(geodesk PRIVATE GEODESK_EXPORTS)
else()
    message(STATUS "GeoDesk: Building static library")
endif()

message(STATUS "GeoDesk: Extra include paths: ${GEODESK_EXTRA_INCLUDE_DIRS}")
target_include_directories(geodesk PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include> ${INCLUDES})
target_include_directories(geodesk PRIVATE ${GEODESK_EXTRA_INCLUDE_DIRS} src ${boost_crc_SOURCE_DIR}/include)

target_link_libraries(geodesk PUBLIC gtl)

# Define where the library and public headers should be installed
install(TARGETS geodesk
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ 
        DESTINATION include 
        FILES_MATCHING PATTERN "*.h")

if(NOT GEODESK_PYTHON AND NOT BUILD_SHARED_LIBS)
    # Define a simple test target (optional)
    enable_testing()
    file(GLOB_RECURSE TEST_SOURCE_FILES "test/*.cpp")
    add_executable(geodesk-test ${TEST_SOURCE_FILES})
    # add_executable(geodesk-test test/test_main.cpp test/test_concur.cpp)
    target_include_directories(geodesk-test PRIVATE include src ${Catch2_SOURCE_DIR})
    target_link_libraries(geodesk-test geodesk Catch2::Catch2WithMain)  # or zlib for shared
    add_test(NAME geodesk-test COMMAND geodesk-test)

    if(GEODESK_EXAMPLES)
        add_subdirectory(examples)
    endif()
endif()

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER}")
message(STATUS "MSVC_VERSION: ${MSVC_VERSION}")