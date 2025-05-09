cmake_minimum_required(VERSION 3.31)

project(Fman)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(FMAN_TESTS "Enable tests project" ON)

set(SOURCES
	include/FileManager/Serializable.hpp
	include/FileManager/FileManager.hpp
	include/FileManager/internal/FileManager_internal.hpp
	include/FileManager/internal/Definitions.hpp
	include/FileManager/internal/pch.hpp
	include/FileManager/vfs/Memoryfs.hpp
	include/FileManager/vfs/MemoryFile.hpp
	
	src/FileManager.cpp
	src/internal/FileManager_internal.cpp
	src/vfs/Memoryfs.cpp
	src/vfs/MemoryFile.cpp

)


add_library(zlib STATIC IMPORTED)
set_target_properties(zlib PROPERTIES
	IMPORTED_LOCATION ${library_zlib}
	INTERFACE_INCLUDE_DIRECTORIES ${include_dir_zlib}
)


add_library(${PROJECT_NAME} STATIC)

target_precompile_headers(${PROJECT_NAME} PRIVATE include/FileManager/internal/pch.hpp)


target_sources(${PROJECT_NAME}
	PRIVATE ${SOURCES}
)

target_include_directories(${PROJECT_NAME}
	PUBLIC include
	PRIVATE include/FileManager
)

target_link_libraries(${PROJECT_NAME} 
	PRIVATE ludutils
	PRIVATE zlib
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

if (FMAN_TESTS)
	include(FetchContent)

	FetchContent_Declare(
		Catch2
		GIT_REPOSITORY https://github.com/catchorg/Catch2.git
		GIT_TAG        v3.4.0
		EXCLUDE_FROM_ALL
	)

	FetchContent_MakeAvailable(Catch2)

	list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
	include(CTest)
	include(Catch)

	set(fman_test_dir ${CMAKE_CURRENT_LIST_DIR}/tests)

	add_executable(FMAN_TESTS)
	target_sources(FMAN_TESTS PRIVATE
		${fman_test_dir}/test_vfs.cpp
		${fman_test_dir}/test.cpp
	)

	target_link_libraries(FMAN_TESTS
		PRIVATE Fman
		PRIVATE Catch2::Catch2WithMain
		PRIVATE ludutils
		PRIVATE zlib
	)
	catch_discover_tests(FMAN_TESTS)

endif()

