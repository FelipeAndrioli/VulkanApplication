cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include_directories(${VK_APP_INCLUDES})

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)

message("Configuring shaders")
set(SHADERS_PATH ${PROJECT_SOURCE_DIR}/src/Assets/Shaders)
set(TEXTURE_PATH ${PROJECT_SOURCE_DIR}/src/Assets/Textures)
message("Shaders Path: ${SHADERS_PATH}")

file(GLOB_RECURSE SHADERS ${SHADERS_PATH}/*.spv)
message("Shaders: ${SHADERS}")

file(COPY ${SHADERS} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Shaders/)
file(COPY ${TEXTURE_PATH}/error_texture.jpg DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Textures/)
file(COPY ${TEXTURE_PATH}/piazza_bologni_1k.hdr DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Textures/)
file(COPY ${TEXTURE_PATH}/immenstadter_horn_2k.hdr DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Textures/)
file(COPY ${TEXTURE_PATH}/back.jpg DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Textures/)
file(COPY ${TEXTURE_PATH}/bottom.jpg DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Textures/)
file(COPY ${TEXTURE_PATH}/front.jpg DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Textures/)
file(COPY ${TEXTURE_PATH}/left.jpg DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Textures/)
file(COPY ${TEXTURE_PATH}/right.jpg DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Textures/)
file(COPY ${TEXTURE_PATH}/top.jpg DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Textures/)

list(APPEND SOURCE_FILES ${PROJECT_SOURCE_DIR}/libs/imgui/imgui.cpp)
list(APPEND SOURCE_FILES ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_demo.cpp)
list(APPEND SOURCE_FILES ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_draw.cpp)
list(APPEND SOURCE_FILES ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_tables.cpp)
list(APPEND SOURCE_FILES ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_widgets.cpp)
list(APPEND SOURCE_FILES ${PROJECT_SOURCE_DIR}/libs/imgui/backends/imgui_impl_glfw.cpp)
list(APPEND SOURCE_FILES ${PROJECT_SOURCE_DIR}/libs/imgui/backends/imgui_impl_vulkan.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} glm)
target_link_libraries(${PROJECT_NAME} glfw)
target_link_libraries(${PROJECT_NAME} assimp)

if (RUNTIME_SHADER_COMPILE)
	target_compile_definitions(${PROJECT_NAME} PRIVATE RUNTIME_SHADER_COMPILATION=${RUNTIME_SHADER_COMPILE})
	target_link_libraries(${PROJECT_NAME} Vulkan::Headers)
	target_link_libraries(${PROJECT_NAME} Vulkan::glslang)
	target_link_libraries(${PROJECT_NAME} Vulkan::SPIRV-Tools)
endif()

target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)
