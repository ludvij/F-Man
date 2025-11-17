// #include "internal/pch.hpp"

#include "FileManager/FileManager.hpp"
#include "FileManager/Serializable.hpp"
#include "FileManager/util/CompressionStreams.hpp"

#include <cstddef>
#include <ios>
#include <istream>
#include <ludutils/lud_assert.hpp>
#include <memory>
#include <system_error>

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

        std::error_code ec;
        fs::create_directories(context.root, ec);
        if (ec)
        {
            // should an exception be thrown here?
            return false;
        }
        return true;
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

        std::error_code ec;
        fs::create_directories(context.current_folder, ec);
        if (ec)
        {
            // should an exception be thrown here?
            return false;
        }
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

std::shared_ptr<std::fstream> PushFile(const fs::path& name, OpenMode mode)
{
    fs::path path = context.current_folder;
    path.append(name.string());
    if (!fs::exists(path) && (mode & mode::read))
    {
        return nullptr;
    }

    return std::make_shared<std::fstream>(path, static_cast<std::ios_base::openmode>(mode));
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

    CompressionOstream deflate_ostream(*file);
    serial->Serialize(deflate_ostream);
}

void DeserializeDecompress(ISerializable* serial)
{
    auto file = PushFile(context.serialize_filename, mode::binary | mode::read);
    Lud::check::that(file != nullptr, "Deserialization file was not found");
    Lud::check::that(file->is_open(), "Could not open deserialization file");

    DecompressionIstream inflate_istream(*file);
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

namespace Resources
{
std::shared_ptr<std::istream> Push(const std::string_view path)
{
#ifndef FMAN_EMBED_RESOURCES
    auto file = context.resources.Get(path);
    Lud::check::that(file != nullptr, "Resource was not found");
#else
    Lud::check::that(Fman::PushFolder(Fman::GetRoot(), false), "Resources folder was not found");
    auto file = Fman::PushFile(path, mode::read);
    Fman::PopFolder();

    Lud::check::that(file != nullptr, "Resource was not found");
    Lud::check::that(file->is_open(), "Could not open file");
#endif
    return file;
}

} // namespace Resources

} // namespace Fman
