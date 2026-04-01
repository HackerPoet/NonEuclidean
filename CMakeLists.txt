cmake_minimum_required(VERSION 3.15)
project(NonEuclidean CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use static MSVC runtime to match original project settings
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Download GLEW 2.1.0 prebuilt Windows binaries
set(GLEW_ROOT "${CMAKE_BINARY_DIR}/_deps/glew-src")
if(NOT EXISTS "${GLEW_ROOT}/include/GL/glew.h")
    set(_glew_zip "${CMAKE_BINARY_DIR}/glew-2.1.0-win32.zip")
    message(STATUS "Downloading GLEW 2.1.0 prebuilt binaries...")
    file(DOWNLOAD
        "https://github.com/nigels-com/glew/releases/download/glew-2.1.0/glew-2.1.0-win32.zip"
        "${_glew_zip}"
        SHOW_PROGRESS
        STATUS _glew_status
    )
    list(GET _glew_status 0 _glew_error)
    if(_glew_error)
        message(FATAL_ERROR "Failed to download GLEW: ${_glew_status}")
    endif()
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/_deps/glew-extract")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf "${_glew_zip}"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/_deps/glew-extract"
        RESULT_VARIABLE _glew_extract_result
    )
    if(_glew_extract_result)
        message(FATAL_ERROR "Failed to extract GLEW")
    endif()
    file(RENAME "${CMAKE_BINARY_DIR}/_deps/glew-extract/glew-2.1.0" "${GLEW_ROOT}")
    file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/_deps/glew-extract")
    file(REMOVE "${_glew_zip}")
endif()

set(GLEW_ROOT "${GLEW_ROOT}")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(GLEW_LIB_DIR "${GLEW_ROOT}/lib/Release/x64")
else()
    set(GLEW_LIB_DIR "${GLEW_ROOT}/lib/Release/Win32")
endif()

add_subdirectory(NonEuclidean)
