cmake_minimum_required(VERSION 3.31)

project(varf)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(VARF_TESTS "Enable tests project" OFF)
option(VARF_DO_CRC32 "Check crc32 on unzip" ON)
option(VARF_EMBED_RESOURCES "Embed resources as a zip file in source" OFF)

set(VARF_PREFERRED_SEPARATOR "/")
set(VARF_RESOURCES_PATH "resources/")


add_library(${PROJECT_NAME} STATIC)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.hpp)

set(SOURCES
	include/FileManager/FileManager.hpp
	include/FileManager/Archive.hpp
	include/FileManager/Serializable.hpp
	include/FileManager/archive/zip.hpp
	include/FileManager/archive/rezip.hpp
	include/FileManager/vfs/Vfs.hpp
	
	src/FileManager.cpp
	src/Serializable.cpp
	src/archive/zip.cpp
	src/archive/rezip.cpp
	src/vfs/Vfs.cpp
	src/FileManager_internal.hpp
	src/FileManager_internal.cpp
	src/pch.hpp
)
target_compile_definitions(${PROJECT_NAME} PRIVATE VARF_PREFERRED_SEPARATOR="${VARF_PREFERRED_SEPARATOR}")

if(VARF_DO_CRC32)
	target_compile_definitions(${PROJECT_NAME} PUBLIC VARF_DO_CRC_32)
endif()

target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})

target_include_directories(${PROJECT_NAME}
	PUBLIC  ${CMAKE_CURRENT_LIST_DIR}/include
	PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(${PROJECT_NAME} 
	PRIVATE ludutils
	PRIVATE compression_streams
)

if(generator_config MATCHES "DEBUG")
	target_compile_definitions(${PROJECT_NAME} PRIVATE VARF_DEBUG)
elseif(generator_config MATCHES "RELEASE")
	target_compile_definitions(${PROJECT_NAME} PRIVATE VARF_RELEASE)
endif()

if(CMAKE_SYSTEM_NAME MATCHES Linux)
	target_compile_definitions(${PROJECT_NAME} PRIVATE VARF_PLATFORM_LINUX)
elseif(CMAKE_SYSTEM_NAME MATCHES Windows)
	target_compile_definitions(${PROJECT_NAME} PRIVATE VARF_PLATFORM_WINDOWS)
else()
	message(SEND_ERROR "platform ${CMAKE_SYSTEM_NAME} is not supported")
endif()

if(VARF_EMBED_RESOURCES)
	add_executable(embed_resources)
	target_sources(embed_resources PRIVATE 
		src/archive/rezip.cpp
		src/scripts/embed_resources.cpp
		src/pch.hpp
	)

	file(GLOB_RECURSE resource_files ${CMAKE_SOURCE_DIR}/${VARF_RESOURCES_PATH}/*)

	target_precompile_headers(embed_resources PRIVATE src/pch.hpp)

	target_include_directories(embed_resources
		PUBLIC  ${CMAKE_CURRENT_LIST_DIR}/include
		PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src
	)

	target_link_libraries(embed_resources
		PRIVATE ludutils
		PRIVATE compression_streams
	)

	set(VARF_GENERATED_RESOURCE generated_resources.cpp)

	add_custom_command(OUTPUT ${VARF_GENERATED_RESOURCE}
		DEPENDS ${resource_files} 
		COMMAND 
			${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/embed_resources 
			${VARF_RESOURCES_PATH} 
			${CMAKE_CURRENT_BINARY_DIR}/${VARF_GENERATED_RESOURCE}
			RESOURCES_BINDUMP
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	)
	add_custom_target(generated_files
		DEPENDS ${VARF_GENERATED_RESOURCE}
	)

	add_dependencies(generated_files embed_resources)
	add_dependencies(${PROJECT_NAME} generated_files)

	target_sources(${PROJECT_NAME} PRIVATE ${VARF_GENERATED_RESOURCE})
	target_compile_definitions(${PROJECT_NAME} PRIVATE VARF_EMBED_RESOURCES)
	target_compile_definitions(${PROJECT_NAME} PUBLIC VARF_RESOURCES_PATH="")
else()
	target_compile_definitions(${PROJECT_NAME} PUBLIC VARF_RESOURCES_PATH="${VARF_RESOURCES_PATH}")
endif()

if (VARF_TESTS)

	FetchContent_Declare(
		Catch2
		GIT_REPOSITORY https://github.com/catchorg/Catch2.git
		GIT_TAG        v3.11.0
		EXCLUDE_FROM_ALL
	)

	FetchContent_MakeAvailable(Catch2)

	set(fman_test_dir ${CMAKE_CURRENT_LIST_DIR}/tests)

	add_executable(VARF_TESTS)
	target_sources(VARF_TESTS PRIVATE
		${fman_test_dir}/test_filemanager.cpp
		${fman_test_dir}/test_serialization.cpp
		${fman_test_dir}/test_vfs.cpp
		${fman_test_dir}/test_unzip.cpp
		${fman_test_dir}/test_zip_file.cpp
	)

	target_link_libraries(VARF_TESTS
		PRIVATE varf
		PRIVATE Catch2::Catch2WithMain
		PRIVATE ludutils
		PRIVATE compression_streams
	)

	list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
	include(CTest)
	include(Catch)
	catch_discover_tests(VARF_TESTS)

endif()

