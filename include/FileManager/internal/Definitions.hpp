#ifndef FILEMANAGER_DEFINITIONS_HEADER
#define FILEMANAGER_DEFINITIONS_HEADER

#include <filesystem>
#include <string>

#if defined(FILEMANAGER_PLATFORM_WINDOWS)
#elif defined(FILEMANAGER_PLATFORM_LINUX)
#endif

namespace Fman
{
	constexpr std::string PLATFORM_DELIMITER(1, std::filesystem::path::preferred_separator);
}

#endif	