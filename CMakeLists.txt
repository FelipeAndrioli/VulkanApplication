cmake_minimum_required(VERSION 3.25.0)
project(VulkanApplication)

option(BUILD_ASSIMP "Build ASSIMP library" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Input/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Assets/Utils/*.cpp
)

file(GLOB_RECURSE HEADER_FILES 
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Input/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/Assets/Utils/*.h
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/glfw)
target_link_libraries(${PROJECT_NAME} glfw)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/glm)
target_link_libraries(${PROJECT_NAME} glm)

if(BUILD_ASSIMP)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/assimp)
	target_link_libraries(${PROJECT_NAME} assimp)
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set_property(TARGET ${PROJECT_NAME} PROPERTY RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/bin/Debug)
set_property(TARGET ${PROJECT_NAME} PROPERTY RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/Release)
set_property(TARGET ${PROJECT_NAME} PROPERTY OUTPUT_NAME "VulkanApplication")