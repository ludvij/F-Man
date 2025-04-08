cmake_minimum_required(VERSION 3.31)

project(Fman)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)


set(SOURCES
	include/FileManager/FileManager.hpp
	include/FileManager/internal/FileManager_internal.hpp
	include/FileManager/Serializable.hpp
	include/FileManager/Memoryfs.hpp

	src/FileManager.cpp
	src/internal/FileManager_internal.cpp
)

set(HEADERS
)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
	PRIVATE ${SOURCES}
)

target_include_directories(${PROJECT_NAME}
	PUBLIC include
	PRIVATE include/FileManager
)

target_link_libraries(${PROJECT_NAME} 
	PRIVATE ludutils
)

if(generator_config MATCHES "DEBUG")
	target_compile_definitions(${PROJECT_NAME} PRIVATE FILEMANAGER_DEBUG)
elseif(generator_config MATCHES "RELEASE")
	target_compile_definitions(${PROJECT_NAME} PRIVATE FILEMANAGER_RELEASE)
endif()

if(CMAKE_SYSTEM_NAME MATCHES Linux)
	target_compile_definitions(${PROJECT_NAME} PRIVATE FILEMANAGER_PLATFORM_LINUX)
elseif(CMAKE_SYSTEM_NAME MATCHES Windows)
	target_compile_definitions(${PROJECT_NAME} PRIVATE FILEMANAGER_PLATFORM_WINDOWS)
else()
	message(SEND_ERROR "platform ${CMAKE_SYSTEM_NAME} is not supported")
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE CMAKE_EXPORT_COMPILE_COMMANDS=1)
