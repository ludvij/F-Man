// #include "pch.hpp"

#include "FileManager_internal.hpp"
#include "archive/rezip.hpp"
#ifdef VARF_PLATFORM_WINDOWS
    #include <ShlObj.h>
    #include <Windows.h>
#endif

#ifdef VARF_EMBED_RESOURCES

extern unsigned char RESOURCES_BINDUMP[]; // NOLINT
extern size_t RESOURCES_BINDUMP_len;

#endif

varf::_detail_::Context::Context()
{
#if defined(VARF_PLATFORM_WINDOWS)
    // I need to clean this
    const auto add_knonw_path = [&](const std::string&& name, GUID id) {
        PWSTR path;

        auto err = SHGetKnownFolderPath(id, KF_FLAG_CREATE, NULL, &path);
        if (err != S_OK)
        {
            CoTaskMemFree(path);
            throw std::runtime_error("path not found");
        }
        known_paths.emplace(name, path);

        CoTaskMemFree(path);
    };
    add_knonw_path("HOME", FOLDERID_Profile);
    add_knonw_path("APPDATA", FOLDERID_RoamingAppData);
    add_knonw_path("DOCUMENTS", FOLDERID_Documents);

#elif defined(VARF_PLATFORM_LINUX)

    known_paths.emplace("HOME", "~");
    known_paths.emplace("APPDATA", "~");
    known_paths.emplace("DOCUMENTS", "~/Documents");

#else
    #error Platform not supported
#endif

    known_paths.emplace("TEMP", std::filesystem::temp_directory_path());
    known_paths.emplace("PWD", std::filesystem::current_path());

    folders.emplace_back(std::filesystem::current_path());

#ifdef VARF_EMBED_RESOURCES

    Lud::memory_istream stream({RESOURCES_BINDUMP, RESOURCES_BINDUMP_len});
    RezipArchive archive(stream);
    resources.LoadArchive(archive);
#endif
}
