cmake_minimum_required(VERSION 3.31)

project(Fman)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(FMAN_TESTS "Enable tests project" OFF)

set(SOURCES
	include/FileManager/Serializable.hpp
	include/FileManager/FileManager.hpp
	
	include/FileManager/internal/FileManager_internal.hpp
	include/FileManager/internal/pch.hpp
	
	
	include/FileManager/util/compression_streams.hpp
	
	src/FileManager.cpp
	src/Serializable.cpp

	src/internal/FileManager_internal.cpp

	src/util/compression_streams.cpp

)



SET(ZLIB_USE_STATIC_LIBS ON)
SET(ZLIB_BUILD_EXAMPLES OFF)


SET(ZLIB_COMPAT ON)
SET(ZLIB_ENABLE_TESTS OFF)
SET(WITH_GTEST OFF)

include(FetchContent)
FetchContent_Declare(
	zlib_ng
	GIT_REPOSITORY https://github.com/zlib-ng/zlib-ng.git
	GIT_TAG        2.2.5
	EXCLUDE_FROM_ALL
)

add_compile_options(-Wno-type-limits)
FetchContent_MakeAvailable(zlib_ng)
add_compile_options(-Wtype-limits)


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
	PRIVATE zlibstatic
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

	FetchContent_Declare(
		Catch2
		GIT_REPOSITORY https://github.com/catchorg/Catch2.git
		GIT_TAG        v3.11.0
		EXCLUDE_FROM_ALL
	)

	FetchContent_MakeAvailable(Catch2)

	set(fman_test_dir ${CMAKE_CURRENT_LIST_DIR}/tests)

	add_executable(FMAN_TESTS)
	target_sources(FMAN_TESTS PRIVATE
		${fman_test_dir}/test_serialization.cpp
	)

	target_link_libraries(FMAN_TESTS
		PRIVATE Fman
		PRIVATE Catch2::Catch2WithMain
		PRIVATE ludutils
		PRIVATE zlibstatic
	)

	list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
	include(CTest)
	include(Catch)
	catch_discover_tests(FMAN_TESTS)

endif()

