// #include "pch.hpp"

#include "FileManager_internal.hpp"
#include "FileManager/comp/archive/rezip.hpp"
#ifdef FILEMANAGER_PLATFORM_WINDOWS
    #include <ShlObj.h>
#endif

#ifdef FMAN_EMBED_RESOURCES

    #include "ludutils/lud_mem_stream.hpp"
extern unsigned char RESOURCES_BINDUMP[]; // NOLINT
extern size_t RESOURCES_BINDUMP_len;

#endif

Fman::_detail_::Context::Context()
{
#if defined(FMAN_PLATFORM_WINDOWS)
    // I need to clean this
    auto add_knonw_path = [&](const std::string& name, GUID id) {
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

#elif defined(FMAN_PLATFORM_LINUX)

    known_paths.emplace("HOME", "~");
    known_paths.emplace("APPDATA", "~");
    known_paths.emplace("DOCUMENTS", "~/Documents");

#else
    #error Platform not supported
#endif

    known_paths.emplace("TEMP", std::filesystem::temp_directory_path());
    known_paths.emplace("PWD", std::filesystem::current_path());

    folders.emplace_back(std::filesystem::current_path());

#ifdef FMAN_EMBED_RESOURCES

    Lud::memory_istream stream({RESOURCES_BINDUMP, RESOURCES_BINDUMP_len});
    comp::RezipArchive archive(stream);
    resources.LoadArchive(archive);
#endif
}
