cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include_directories(${VK_APP_INCLUDES})

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)


list(APPEND SOURCE_FILES ${PROJECT_SOURCE_DIR}/libs/imgui/imgui.cpp)
list(APPEND SOURCE_FILES ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_demo.cpp)
list(APPEND SOURCE_FILES ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_draw.cpp)
list(APPEND SOURCE_FILES ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_tables.cpp)
list(APPEND SOURCE_FILES ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_widgets.cpp)
list(APPEND SOURCE_FILES ${PROJECT_SOURCE_DIR}/libs/imgui/backends/imgui_impl_glfw.cpp)
list(APPEND SOURCE_FILES ${PROJECT_SOURCE_DIR}/libs/imgui/backends/imgui_impl_vulkan.cpp)

add_executable(VulkanApplication ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} glm)
target_link_libraries(${PROJECT_NAME} glfw)
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARY})
