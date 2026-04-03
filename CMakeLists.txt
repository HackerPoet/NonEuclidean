cmake_minimum_required(VERSION 3.15)
project(NonEuclidean CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use static MSVC runtime to match original project settings
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)

# Vulkan Headers - fetch first so volk and glslang can find them
FetchContent_Declare(vulkan-headers
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG vulkan-sdk-1.3.275.0
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(vulkan-headers)

# Tell volk and others where to find vulkan headers
set(VULKAN_HEADERS_INSTALL_DIR "${vulkan-headers_SOURCE_DIR}" CACHE PATH "" FORCE)
set(Vulkan_INCLUDE_DIR "${vulkan-headers_SOURCE_DIR}/include" CACHE PATH "" FORCE)

# Volk - Vulkan meta-loader (no vulkan-1.lib needed)
set(VOLK_INSTALL OFF CACHE BOOL "" FORCE)
set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR CACHE STRING "" FORCE)
FetchContent_Declare(volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG vulkan-sdk-1.3.275.0
    GIT_SHALLOW TRUE)

# glslang - runtime GLSL to SPIR-V compilation
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(ENABLE_CTEST OFF CACHE BOOL "" FORCE)
set(ENABLE_HLSL OFF CACHE BOOL "" FORCE)
set(ENABLE_OPT OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
    GIT_TAG 14.0.0
    GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(volk glslang)

add_subdirectory(NonEuclidean)

# ---- Portal warp unit test (no Vulkan dependency) -------------------------
add_executable(portal_warp_test tests/portal_warp_test.cpp)
target_include_directories(portal_warp_test PRIVATE NonEuclidean/game)
if(MSVC)
    target_compile_options(portal_warp_test PRIVATE /W4 /WX)
endif()
enable_testing()
add_test(NAME portal_warp COMMAND portal_warp_test)

if(WIN32)
    add_test(
        NAME level8_portal_illumination
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass
            -File ${CMAKE_SOURCE_DIR}/tests/level8_illumination.ps1
            $<TARGET_FILE:NonEuclidean>
            ${CMAKE_SOURCE_DIR}/NonEuclidean
            ${CMAKE_BINARY_DIR}/level8_portal_illumination.png)
    set_tests_properties(level8_portal_illumination PROPERTIES TIMEOUT 30)
endif()
