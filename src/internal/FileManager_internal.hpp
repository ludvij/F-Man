#ifndef FILE_MANAGER_INTERNAL_HEADER
#define FILE_MANAGER_INTERNAL_HEADER

#ifdef FMAN_EMBED_RESOURCES
    #include "FileManager/vfs/Vfs.hpp"
#endif
#include <deque>
#include <filesystem>
#include <unordered_map>
#include <vector>

namespace Fman::_detail_
{

class Context
{
public:
    Context();
    ~Context();

    Context(const Context& other) = delete;
    Context& operator=(const Context& other) = delete;

    Context(Context&& other) = delete;
    Context& operator=(Context&& other) = delete;

private:
public:

    std::string serialize_filename = "srl.dat";

    std::filesystem::path root;
    std::filesystem::path current_folder;

    std::unordered_map<std::string, std::filesystem::path> known_paths;
    std::deque<std::filesystem::path> folders;
    std::vector<char*> allocations;

#ifdef FMAN_EMBED_RESOURCES
    VTree resources{VTree::Create()};
#endif

    // std::mutex m_current_file_mutex;
};

} // namespace Fman::_detail_
#endif
