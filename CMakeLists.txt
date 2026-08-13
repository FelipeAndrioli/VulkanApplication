cmake_minimum_required(VERSION 3.24)

project("VulkanApplication")

set(BUILD_SHARED_LIBRARY false CACHE BOOL "Build shared library.")
set(ENABLE_IMGUI true CACHE BOOL "Enable ImGui.")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export Compile Commands" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# CMAKE_EXPORT_COMPILE_COMMANDS will create a 'compile_commands.json' file inside 
# the build directory, the command 'ln -s location/compile_commands.json project/root/directory' 
# must be executed to create a symbolic link and clangd will automatically pick
# up.

set(SRC_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src")
set(ASSETS_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src/Assets")
set(ASSETS_UTILS_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src/Assets/Utils")
set(INPUT_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src/Input")
set(UTILS_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src/Utils")
set(GLFW_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/libs/glfw/include")
set(GLM_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/libs/glm/glm")
set(STB_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/libs/stb")
set(IMGUI_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/libs/imgui")
set(IMGUI_BACKENDS_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/libs/imgui/backends")
set(ASSIMP_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/libs/assimp/include" "${CMAKE_CURRENT_BINARY_DIR}/libs/assimp/include")

set(RUNTIME_SHADER_COMPILE "RUNTIME_SHADER_COMPILE" CACHE STRING "TRUE")

if (RUNTIME_SHADER_COMPILE)
	message("RUNTIME SHADER COMPILATION ENABLED")
	find_package(Vulkan REQUIRED COMPONENTS glslang shaderc_combined SPIRV-Tools)

	if (NOT Vulkan_glslang_FOUND)
		message("FATAL ERROR - glslang not found!")
	endif()

	if (NOT Vulkan_SPIRV-Tools_FOUND)
		message("FATAL ERROR - SPIRV-Tools not found!")
	endif()

	if (NOT Vulkan_shaderc_combined_FOUND)
		message("FATAL ERROR - shaderc_combined not found!")
	endif()

else()
	message("RUNTIME SHADER COMPILATION DISABLED")
	find_package(Vulkan REQUIRED)
endif()

if (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Vulkan not found!")
endif()

set(VK_APP_INCLUDES ${APP_INCLUDE_DIRS}
	${ASSETS_INCLUDE_DIRS}
	${ASSETS_UTILS_INCLUDE_DIRS}
	${INPUT_INCLUDE_DIRS}
	${UTILS_INCLUDE_DIRS}
	${GLFW_INCLUDE_DIRS}
	${GLM_INCLUDE_DIRS}
	${STB_INCLUDE_DIRS}
	${IMGUI_INCLUDE_DIRS}
	${IMGUI_BACKENDS_INCLUDE_DIRS}
	${ASSIMP_INCLUDE_DIRS}
	${Vulkan_INCLUDE_DIR})

add_subdirectory(${PROJECT_SOURCE_DIR}/libs/glm)
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/glfw)

set(ASSIMP_WARNINGS_AS_ERRORS OFF)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "")
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "")
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "")
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "")
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/assimp)

add_subdirectory(src)
