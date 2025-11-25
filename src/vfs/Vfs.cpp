#include "FileManager/vfs/Vfs.hpp"
#include "FileManager/FileManager.hpp"
#include "FileManager/comp/Archive.hpp"

namespace varf {

namespace fs = std::filesystem;

VTree VTree::Create()
{
    return {};
}

size_t VTree::Load(const fs::path& path)
{
    Lud::check::that(varf::PushFolder(path), "path not found");
    auto paths = varf::Traverse({.depth = varf::TRAVERSAL_FULL});
    varf::PopFolder();

    size_t elems = 0;
    for (const auto& elem : paths)
    {
        if (fs::is_directory(elem))
        {
            elems += VTree::Add(elem.string());
        }
        else
        {
            const auto slurp = [](const auto& file_name) {
                std::ifstream stream(file_name, std::ios::binary | std::ios::ate);
                const std::streamsize sz = stream.tellg();

                stream.seekg(0, std::ios::beg);

                std::vector<uint8_t> vec(sz);
                stream.read(reinterpret_cast<char*>(vec.data()), sz);

                return vec;
            };
            elems += VTree::Add(elem.string(), slurp(elem));
        }
    }
    return elems;
}

size_t VTree::LoadArchive(const comp::Archive& archive)
{
    auto directory = archive.GetDirectory();
    size_t elems = 0;

    for (const auto& entry : directory)
    {
        // just a folder
        if (entry.file_name.ends_with(VARF_PREFERRED_SEPARATOR))
        {
            elems += VTree::Add(entry.file_name);
        }
        else
        {
            elems += VTree::Add(entry.file_name, archive.Peek(entry));
        }
    }
    return elems;
}

bool VTree::Add(const std::string_view path)
{
    if (path.empty())
    {
        return false;
    }
    const auto parts = Lud::Split(Lud::ToLower(path), VARF_PREFERRED_SEPARATOR);
    Node* last = &m_root;
    for (const auto& part : parts | std::views::take(parts.size() - 1))
    {
        auto& variant = last->children[part];
        if (last = std::get_if<Node>(&variant); last == nullptr)
        {
            return false;
        }
    }
    if (last->children.contains(parts.back()))
    {
        return false;
    }
    last->children.emplace(parts.back(), std::in_place_type<Node>);
    return true;
}

bool VTree::Add(const std::string_view path, std::vector<uint8_t>&& data)
{
    if (path.empty())
    {
        return false;
    }
    const auto parts = Lud::Split(Lud::ToLower(path), VARF_PREFERRED_SEPARATOR);
    Node* last = &m_root;
    for (const auto& part : parts | std::views::take(parts.size() - 1))
    {
        auto& variant = last->children[part];
        if (last = std::get_if<Node>(&variant); last == nullptr)
        {
            return false;
        }
    }
    if (last->children.contains(parts.back()))
    {
        return false;
    }
    last->children.emplace(parts.back(), std::move(data));
    return true;
}

bool VTree::Contains(const std::string_view path) const
{
    if (path.empty())
    {
        return false;
    }
    const std::vector<std::string> parts = Lud::Split(path, VARF_PREFERRED_SEPARATOR);
    const Node* last = &m_root;
    for (const auto& part : parts)
    {
        const auto it = last->children.find(part);
        if (it == last->children.end())
        {
            return false;
        }
        if (last = std::get_if<Node>(&it->second); last == nullptr)
        {
            return false;
        }
    }
    return true;
}

bool VTree::Remove(const std::string_view path)
{
    if (path.empty())
    {
        return false;
    }
    const std::vector<std::string> parts = Lud::Split(path, VARF_PREFERRED_SEPARATOR);

    Node* last = &m_root;
    for (const auto& part : parts | std::views::take(parts.size() - 1))
    {
        const auto it = last->children.find(part);
        if (it == last->children.end())
        {
            return false;
        }
        if (last = std::get_if<Node>(&it->second); last == nullptr)
        {
            return false;
        }
    }

    return last->children.erase(parts.back()) != 0;
}

std::shared_ptr<std::istream> VTree::Get(const std::string_view path)
{
    if (path.empty())
    {
        return nullptr;
    }
    const std::vector<std::string> parts = Lud::Split(path, VARF_PREFERRED_SEPARATOR);

    Node* last = &m_root;
    for (const auto& part : parts | std::views::take(parts.size() - 1))
    {
        const auto it = last->children.find(part);
        if (it == last->children.end())
        {
            return nullptr;
        }
        if (last = std::get_if<Node>(&it->second); last == nullptr)
        {
            return nullptr;
        }
    }
    // last iteration outside of the loop, I would love to do it inside,
    // as this is very ugly, but hey, it works!
    const auto it = last->children.find(parts.back());
    if (it == last->children.end())
    {
        return nullptr;
    }
    auto vfile = std::get_if<VFile>(&it->second);
    if (vfile == nullptr)
    {
        return nullptr;
    }
    return std::make_shared<Lud::memory_istream<uint8_t>>(vfile->data);
}

VTree::VFile::VFile(std::vector<uint8_t>&& vec_data)
    : data(std::move(vec_data))
{
}

} // namespace varf
