#include "FileManager.hpp"
#include "FileManager_internal.hpp"
#include "Serializable.hpp"

#include <filesystem>

#include <comp_streams/CompStreams.hpp>

namespace fs = std::filesystem;
namespace ranges = std::ranges;

namespace varf {
_detail_::Context context; // NOLINT

using Lud::deflate_ostream;
using Lud::inflate_istream;

static void set_current_working_dir(const fs::path& path)
{
    try
    {
        fs::current_path(path);
    }
    catch (const fs::filesystem_error& e)
    {
        varf::Reset();
        throw e;
    }
}

static void validate_path(const fs::path& path)
{
    constexpr static std::string_view invalid = R"(%*?|"<>,;=)";
    const auto& str = path.string();

    Lud::check::is_false(Lud::ContainsAny(str, invalid), "path contains invalid characters");
    Lud::check::is_false(str.ends_with("."), "path can not end in '.'");
    Lud::check::is_false(fs::exists(path) && !fs::is_directory(path), "file already exists with this path");
}

static bool internal_push(const fs::path& name, bool create)
{
    if (name.empty() || name == fs::path{"."})
    {
        return true;
    }
    if (name == fs::path{".."})
    {
        Pop();
        return true;
    }

    Lud::check::is_false(name.is_absolute(), "path can not be absolute");
    validate_path(name);

    if (create)
    {
        fs::create_directories(name);
    }
    else if (!fs::exists(name))
    {
        return false;
    }
    context.folders.emplace_back(name);
    set_current_working_dir(GetCurrent());

    return true;
}

fs::path GetCurrent()
{
    fs::path path;
    for (const auto& elem : context.folders)
    {
        path /= elem;
    }
    return path;
}

fs::path GetRoot()
{
    return context.folders.front();
}

void SetRoot(const fs::path& name)
{
    fs::path preferred = name;
    preferred.make_preferred();
    if (name.empty())
    {
        context.folders.emplace_front(GetCurrent());
        context.folders.erase(context.folders.cbegin() + 1, context.folders.cend());
        return;
    }

    validate_path(name);
    fs::create_directories(name);

    context.folders.clear();
    context.folders.emplace_back(name);
    set_current_working_dir(GetCurrent());
}

void SetRootToKnownPath(const std::string& name)
{
    if (auto path = context.known_paths.find(name); path != context.known_paths.end())
    {
        SetRoot(path->second);
    }
}

void Reset()
{
    SetRootToKnownPath("PWD");
}

bool Push(const fs::path& name, bool create /*= true*/)
{
    fs::path preferred = name;
    preferred.make_preferred();

    const size_t sz = context.folders.size();
    for (const auto& elem : preferred)
    {
        if (!internal_push(elem, create))
        {
            context.folders.resize(sz);
            return false;
        }
    }

    return true;
}

void Pop(size_t amount /* = 1*/)
{
    Lud::check::that(context.folders.size() != 1, "current directory is root");
    Lud::check::that(amount == POP_FULL || amount < context.folders.size(), "Can't pop more folders than pushed amount");

    if (amount == POP_FULL || amount == context.folders.size() - 1)
    {
        context.folders.resize(1);
    }
    else
    {
        context.folders.resize(context.folders.size() - amount);
    }

    set_current_working_dir(GetCurrent());
}

std::shared_ptr<std::fstream> PushFile(const fs::path& name, OpenMode mode)
{
    if (!fs::exists(name) && (mode & mode::read))
    {
        return nullptr;
    }

    auto file = std::make_shared<std::fstream>(name, static_cast<std::ios_base::openmode>(mode));

    if (!file->good() || !file->is_open())
    {
        return nullptr;
    }
    return file;
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

namespace rcs {
std::shared_ptr<std::istream> Get(const std::filesystem::path& path)
{
#ifdef VARF_EMBED_RESOURCES

    fs::path preferred = path;
    preferred.make_preferred();
    auto file = context.resources.Get(preferred.make_preferred().string());

#else

    Lud::check::that(varf::Push(varf::GetRoot(), false), "Resources folder was not found");
    auto file = varf::PushFile(path, mode::read);
    varf::Pop();

#endif

    Lud::check::that(file != nullptr, "Resource was not found");

    return file;
}

} // namespace rcs

} // namespace varf
