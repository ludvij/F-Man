// #include "internal/pch.hpp"

#include "FileManager/FileManager.hpp"
#include "FileManager/Serializable.hpp"
#include "FileManager/util/CompressionStreams.hpp"

#include <ludutils/lud_assert.hpp>

namespace fs = std::filesystem;
namespace ranges = std::ranges;

namespace Fman
{
_detail_::Context context; // NOLINT

using Compression::CompressionOstream;
using Compression::DecompressionIstream;

fs::path GetCurrent()
{
    return context.current_folder;
}

fs::path GetRoot()
{
    return context.root;
}

bool SetRoot(const fs::path& name)
{
    if (name.empty())
    {
        context.root = context.current_folder;
        context.folders.clear();
        return false;
    }
    else
    {
        context.root = context.current_folder = name;
        context.folders.clear();

        return fs::create_directories(context.root);
    }
}

bool SetRootToKnownPath(const std::string& name)
{
    if (auto path = context.known_paths.find(name); path != context.known_paths.end())
    {
        return SetRoot(path->second);
    }
    return false;
}

bool PushFolder(const fs::path& name, bool create /*= true*/)
{
    // folders can not end on period
    Lud::check::that(!name.filename().empty() && name.extension().empty(), "Folders can't have trailing period");
    if (create)
    {
        context.current_folder /= name;
        context.folders.emplace_back(fs::absolute(context.current_folder));

        fs::create_directories(context.current_folder);
    }
    else
    {
        const bool exists = fs::exists(name);
        if (!exists)
        {
            return false;
        }
        context.folders.emplace_back(name);
        context.current_folder /= name;
    }
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

// huh???
char* AllocateFileName(const char* name)
{
    auto current = GetCurrent();
    current.append(name);
    auto repr = current.string();
    size_t sz = repr.size();

    char* s = new char[sz + 1]{0};

    fman_strncpyn(repr.c_str(), sz, s, sz);

    context.allocations.push_back(s);

    return s;
}

void PopFolder(int amount)
{
    size_t u_amount = std::abs(amount);
    Lud::check::leq(u_amount, context.folders.size(), "Can't pop more folders than pushed amount");

    if (amount < 0 || u_amount == context.folders.size())
    {
        context.folders.clear();
        context.folders.push_back(context.root);
    }
    else
    {
        context.folders.resize(context.folders.size() - amount);
    }
    context.current_folder = context.folders.back();
}

std::optional<std::fstream> PushFile(const fs::path& name, OpenMode mode)
{
    fs::path path = context.current_folder;
    path.append(name.string());
    if (!fs::exists(path) && (mode & mode::read))
    {
        return std::nullopt;
    }

    return std::fstream{path, static_cast<std::ios_base::openmode>(mode)};
}

std::vector<std::filesystem::path> Traverse(const int depth, const TraverseMode trav_mode, const std::initializer_list<std::string_view> filters)
{
    Lud::check::ne(depth, 0, "depth must be different from 0");
    std::vector<fs::path> result;

    int curr_depth = 0;
    std::deque<fs::directory_entry> iters;

    iters.emplace_back(context.current_folder);

    size_t size = 1;
    while (!iters.empty())
    {
        fs::directory_iterator it{iters.front()};
        iters.pop_front();
        for (const auto& dir_entry : it)
        {
            if (fs::is_directory(dir_entry))
            {
                if (trav_mode & traverse::folders)
                {
                    result.emplace_back(dir_entry);
                }
                iters.push_back(dir_entry);
            }
            if (!fs::is_directory(dir_entry) && (trav_mode & traverse::files))
            {
                if (filters.size() > 0)
                {
                    const auto ext = dir_entry.path().extension();
                    if (!(ranges::find(filters, ext) != filters.end()))
                    {
                        continue;
                    }
                }
                result.emplace_back(dir_entry);
            }
        }
        if (--size == 0)
        {
            size = iters.size();
            if (depth > FULL && ++curr_depth >= depth)
            {
                return result;
            }
        }
    }
    return result;
}

std::string Slurp(const fs::path& path)
{
    auto file_opt = PushFile(path, mode::read | mode::end);
    if (!file_opt.has_value())
    {
        return {};
    }

    auto& file = file_opt.value();
    const size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    char* buffer = new char[size];
    file.read(buffer, static_cast<std::streamsize>(size));

    std::string res(buffer, size);
    delete[] buffer;

    return res;
}

void Serialize(ISerializable* serial)
{
    auto file = PushFile(context.serialize_filename, mode::binary | mode::write);
    if (!file)
    {
        // idk
    }
    serial->Serialize(file.value());
}

void Deserialize(ISerializable* serial)
{
    auto file = PushFile(context.serialize_filename, mode::binary | mode::read);
    if (!file)
    {
        // idk
    }
    serial->Deserialize(file.value());
}

void SerializeCompress(ISerializable* serial)
{
    auto file = PushFile(context.serialize_filename, mode::binary | mode::write);
    if (!file)
    {
        // idk
    }
    CompressionOstream deflate_ostream(file.value());
    serial->Serialize(deflate_ostream);
}

void DeserializeDecompress(ISerializable* serial)
{
    auto file = PushFile(context.serialize_filename, mode::binary | mode::read);
    if (!file)
    {
        // idk
    }
    DecompressionIstream inflate_istream(file.value());
    serial->Deserialize(inflate_istream);
}

void SetSerializeFilename(const std::string_view name)
{
    context.serialize_filename = name;
}

} // namespace Fman
