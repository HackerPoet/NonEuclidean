cmake_minimum_required(VERSION 3.15)
project(NonEuclidean CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use static MSVC runtime to match original project settings
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)

# Download GLEW 2.1.0 prebuilt Windows binaries
FetchContent_Declare(
    glew
    URL https://github.com/nigels-com/glew/releases/download/glew-2.1.0/glew-2.1.0-win32.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_GetProperties(glew)
if(NOT glew_POPULATED)
    FetchContent_Populate(glew)
endif()

set(GLEW_ROOT "${glew_SOURCE_DIR}")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(GLEW_LIB_DIR "${GLEW_ROOT}/lib/Release/x64")
else()
    set(GLEW_LIB_DIR "${GLEW_ROOT}/lib/Release/Win32")
endif()

add_subdirectory(NonEuclidean)
