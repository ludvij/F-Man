#ifndef VARF_INTERNAL_HEADER
#define VARF_INTERNAL_HEADER

#ifdef VARF_EMBED_RESOURCES
    #include "FileManager/vfs/Vfs.hpp"
#endif

namespace varf::_detail_ {

class Context
{
public:
    Context();

    Context(const Context& other) = delete;
    Context& operator=(const Context& other) = delete;

    Context(Context&& other) = delete;
    Context& operator=(Context&& other) = delete;

private:
public:

    std::string serialize_filename = "srl.dat";

    std::unordered_map<std::string, std::filesystem::path> known_paths;

    // front is root
    // back is current
    std::deque<std::filesystem::path> folders;

#ifdef VARF_EMBED_RESOURCES
    VTree resources{VTree::Create()};
#endif

    // std::mutex m_current_file_mutex;
};

} // namespace varf::_detail_
#endif
