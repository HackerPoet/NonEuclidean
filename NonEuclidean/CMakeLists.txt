set(SOURCES
    engine/Engine.cpp
    engine/Input.cpp
    engine/Main.cpp
    engine/SceneFile.cpp
    engine/world.cpp
    render/Camera.cpp
    render/FrameBuffer.cpp
    render/Mesh.cpp
    render/PortalMeshWarp.cpp
    render/Resources.cpp
    render/Shader.cpp
    render/Texture.cpp
    render/ShadowMap.cpp
    render/VulkanRenderer.cpp
    render/stb_image_impl.cpp
    levels/Level1.cpp
    levels/Level2.cpp
    levels/Level3.cpp
    levels/Level4.cpp
    levels/Level5.cpp
    levels/Level6.cpp
    levels/Level7.cpp
    levels/Level8.cpp
    game/Collider.cpp
    game/ColliderBVH.cpp
    game/Object.cpp
    game/Physical.cpp
    game/Player.cpp
    game/Portal.cpp
    objects/MazeGenerator.cpp
)




# WIN32 sets the subsystem to Windows (WinMain entry point)
add_executable(NonEuclidean WIN32 ${SOURCES})

target_include_directories(NonEuclidean PRIVATE
    # Each subdirectory is added so that unqualified #include "Header.h" still resolves
    "${CMAKE_CURRENT_SOURCE_DIR}/engine"
    "${CMAKE_CURRENT_SOURCE_DIR}/render"
    "${CMAKE_CURRENT_SOURCE_DIR}/levels"
    "${CMAKE_CURRENT_SOURCE_DIR}/game"
    "${CMAKE_CURRENT_SOURCE_DIR}/objects"
    "${stb_SOURCE_DIR}"
)

target_compile_definitions(NonEuclidean PRIVATE
    WIN32_LEAN_AND_MEAN
    VK_USE_PLATFORM_WIN32_KHR
    VK_NO_PROTOTYPES
    $<$<CONFIG:Debug>:_DEBUG>
)

target_link_libraries(NonEuclidean PRIVATE
    volk
    glslang
    SPIRV
    glslang-default-resource-limits
    assimp
)

if(MSVC)
    target_compile_options(NonEuclidean PRIVATE
        /W4
        /GR-            # Disable RTTI (matches original)
        $<$<CONFIG:Debug>:/Od /sdl>
        $<$<CONFIG:Release>:/O2 /Oi /Gy /sdl /GL>
    )
    target_link_options(NonEuclidean PRIVATE
        $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
    )
endif()

# Copy assets to the output directory so relative shader/texture/mesh paths resolve
add_custom_command(TARGET NonEuclidean POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Shaders"
        "$<TARGET_FILE_DIR:NonEuclidean>/Shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/RTShaders"
        "$<TARGET_FILE_DIR:NonEuclidean>/RTShaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Textures"
        "$<TARGET_FILE_DIR:NonEuclidean>/Textures"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Meshes"
        "$<TARGET_FILE_DIR:NonEuclidean>/Meshes"
    COMMENT "Copying assets to output directory"
)

# Set VS debugger working directory to the output folder so assets are found at runtime
set_property(TARGET NonEuclidean PROPERTY
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:NonEuclidean>"
)
