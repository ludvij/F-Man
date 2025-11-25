#include "FileManager/FileManager.hpp"
#include "FileManager/Serializable.hpp"
#include "FileManager/comp/deflate_stream.hpp"
#include "FileManager/comp/inflate_stream.hpp"
#include "FileManager_internal.hpp"

namespace fs = std::filesystem;
namespace ranges = std::ranges;

namespace varf {
_detail_::Context context; // NOLINT

using comp::deflate_ostream;
using comp::inflate_istream;

static void set_current_working_dir(const fs::path& path)
{
    fs::current_path(path);
}

fs::path GetCurrent()
{
    return context.folders.back();
}

fs::path GetRoot()
{
    return context.folders.front();
}

void SetRoot(const fs::path& name)
{
    if (name.empty())
    {
        context.folders.erase(context.folders.cbegin(), context.folders.cend() - 1);
    }
    else
    {
        std::error_code ec;
        fs::create_directories(name, ec);
        if (ec)
        {
            // should an exception be thrown here?
            throw std::runtime_error("Could not create directory");
        }
        context.folders.clear();
        context.folders.emplace_back(name);
    }
    set_current_working_dir(context.folders.front());
}

void SetRootToKnownPath(const std::string& name)
{
    if (auto path = context.known_paths.find(name); path != context.known_paths.end())
    {
        SetRoot(path->second);
    }
}

bool PushFolder(const fs::path& name, bool create /*= true*/)
{
    if (name.empty())
    {
        return true;
    }
    // folders can not end on period
    // TODO: validate pathname
    if (create)
    {
        std::error_code ec;
        fs::create_directories(name, ec);
        if (ec)
        {
            // should an exception be thrown here?
            return false;
        }
    }
    else
    {
        if (!fs::exists(name))
        {
            return false;
        }
    }
    context.folders.emplace_back(name);
    set_current_working_dir(context.folders.back());
    return true;
}

bool PushFolder(std::initializer_list<fs::path> name, bool create /*= true*/)
{
    for (const auto& f : name)
    {
        if (!PushFolder(f, create))
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief copies min(src_sz, dst_sz) chars from src to dst
 * @param src ptr to source cstring
 * @param src_sz size of source cstring without null terminator
 * @param dst ptr to destination cstring
 * @param dst_sz size of destination cstring without null terminator
 */
static void fman_strncpyn(const char* src, const size_t src_sz, char* dst, const size_t dst_sz)
{
    if (src == nullptr || dst == nullptr)
    {
        return;
    }

    const size_t min_sz = src_sz > dst_sz ? dst_sz : src_sz;

    memmove(dst, src, min_sz);

    dst[dst_sz] = '\0';
}

void PopFolder(int amount /* = 1*/)
{
    if (amount > 0)
    {
        Lud::check::leq(static_cast<size_t>(amount), context.folders.size(), "Can't pop more folders than pushed amount");
    }

    if (amount < 0 || static_cast<size_t>(amount) == context.folders.size())
    {
        context.folders.resize(1);
    }
    else
    {
        context.folders.resize(context.folders.size() - amount);
    }

    set_current_working_dir(context.folders.back());
}

std::shared_ptr<std::fstream> PushFile(const fs::path& name, OpenMode mode)
{
    if (!fs::exists(name) && (mode & mode::read))
    {
        return nullptr;
    }

    return std::make_shared<std::fstream>(name, static_cast<std::ios_base::openmode>(mode));
}

std::vector<std::filesystem::path> Traverse(const TraverseOptions& options)
{
    Lud::check::ne(options.depth, 0, "depth must be different from 0");
    std::vector<fs::path> result;

    int curr_depth = 0;
    std::deque<fs::directory_entry> iters;

    iters.emplace_back(context.folders.back());

    size_t size = 1;
    while (!iters.empty())
    {
        fs::directory_iterator it{iters.front()};
        iters.pop_front();
        for (const auto& dir_entry : it)
        {
            const auto rel = fs::relative(dir_entry, context.folders.back());
            if (fs::is_directory(dir_entry))
            {
                if (options.mode & traverse::folders)
                {
                    result.emplace_back(rel);
                }
                iters.push_back(dir_entry);
            }
            if (!fs::is_directory(dir_entry) && (options.mode & traverse::files))
            {
                if (options.filters.size() > 0)
                {
                    const auto ext = dir_entry.path().extension();
                    if (!(ranges::find(options.filters, ext) != options.filters.end()))
                    {
                        continue;
                    }
                }
                result.emplace_back(rel);
            }
        }
        if (--size == 0)
        {
            size = iters.size();
            if (options.depth > TRAVERSAL_FULL && ++curr_depth >= options.depth)
            {
                return result;
            }
        }
    }
    return result;
}

void Serialize(ISerializable* serial)
{
    auto file = PushFile(context.serialize_filename, mode::binary | mode::write);
    Lud::check::that(file->is_open(), "Could not open serialization file");

    serial->Serialize(*file);
}

void Deserialize(ISerializable* serial)
{
    auto file = PushFile(context.serialize_filename, mode::binary | mode::read);
    Lud::check::that(file != nullptr, "Deserialization file was not found");
    Lud::check::that(file->is_open(), "Could not open deserialization file");

    serial->Deserialize(*file);
}

void SerializeCompress(ISerializable* serial)
{
    auto file = PushFile(context.serialize_filename, mode::binary | mode::write);
    Lud::check::that(file->is_open(), "Could not open serialization file");

    deflate_ostream deflate_ostream(*file);
    serial->Serialize(deflate_ostream);
}

void DeserializeDecompress(ISerializable* serial)
{
    auto file = PushFile(context.serialize_filename, mode::binary | mode::read);
    Lud::check::that(file != nullptr, "Deserialization file was not found");
    Lud::check::that(file->is_open(), "Could not open deserialization file");

    inflate_istream inflate_istream(*file);
    serial->Deserialize(inflate_istream);
}

void SetSerializeFilename(const std::string_view name)
{
    context.serialize_filename = name;
}

std::filesystem::path GetFromCurrent(const std::filesystem::path& path)
{
    return GetCurrent() / path;
}

namespace rcs {
std::shared_ptr<std::istream> Get(const std::string_view path)
{
#ifdef VARF_EMBED_RESOURCES

    auto file = context.resources.Get(path);
    Lud::check::that(file != nullptr, "Resource was not found");

#else

    Lud::check::that(varf::PushFolder(varf::GetRoot(), false), "Resources folder was not found");
    auto file = varf::PushFile(path, mode::read);
    varf::PopFolder();

    Lud::check::that(file != nullptr, "Resource was not found");
    Lud::check::that(file->is_open(), "Could not open file");

#endif

    return file;
}

} // namespace rcs

} // namespace varf
