#include "FileManager/vfs/Vfs.hpp"
#include "FileManager/FileManager.hpp"
#include "FileManager/util/Unzip.hpp"
#include "ludutils/lud_mem_stream.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <istream>
#include <ludutils/lud_parse.hpp>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <variant>

namespace Fman
{

namespace fs = std::filesystem;

VTree VTree::Create()
{
    return {};
}

size_t VTree::LoadFrom(const fs::path& path)
{
    if (path.empty())
    {
        return false;
    }
    Fman::PushFolder(fs::absolute(path));
    auto paths = Fman::Traverse(Fman::FULL);
    size_t size = 0;
    for (const auto& elem : paths)
    {
        if (fs::is_directory(elem))
        {
            VTree::Add(elem.string());
        }
        else
        {
            const auto slurp = [](const auto& file_name)
            {
                std::ifstream stream(file_name, std::ios::binary | std::ios::ate);
                const std::streamsize sz = stream.tellg();

                stream.seekg(0, std::ios::beg);

                std::vector<uint8_t> vec(sz);
                stream.read(reinterpret_cast<char*>(vec.data()), sz);

                return vec;
            };
            VTree::Add(elem.string(), slurp(elem));
        }
        size++;
    }
    Fman::PopFolder();
    return size;
}

size_t VTree::LoadZip(std::istream& stream)
{
    using namespace Fman::Compression;
    auto directory = CreateZipDirectory(stream);
    size_t elems = 0;

    for (const auto& entry : directory)
    {
        // just a folder
        if (entry.file_name.ends_with(FMAN_PREFERRED_SEPARATOR))
        {
            VTree::Add(entry.file_name);
        }
        else
        {
            VTree::Add(entry.file_name, DecompressZippedFile(entry, stream));
        }
        elems++;
    }
    return elems;
}

bool VTree::Add(const std::string_view path)
{
    if (path.empty())
    {
        return false;
    }
    const auto parts = Lud::Split(Lud::ToLower(path), FMAN_PREFERRED_SEPARATOR);
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
    const auto parts = Lud::Split(Lud::ToLower(path), FMAN_PREFERRED_SEPARATOR);
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
    const std::vector<std::string> parts = Lud::Split(path, FMAN_PREFERRED_SEPARATOR);
    const Node* last = &m_root;
    for (const auto& part : parts)
    {
        if (const auto it = last->children.find(part); it != last->children.end())
        {
            if (last = std::get_if<Node>(&it->second); last != nullptr)
            {
                continue;
            }
        }
        return false;
    }
    return true;
}

bool VTree::Remove(const std::string_view path)
{
    if (path.empty())
    {
        return false;
    }
    const std::vector<std::string> parts = Lud::Split(path, FMAN_PREFERRED_SEPARATOR);

    Node* last = &m_root;
    for (const auto& part : parts | std::views::take(parts.size() - 1))
    {
        if (const auto var = last->children.find(part); var != last->children.end())
        {
            if (last = std::get_if<Node>(&var->second); last != nullptr)
            {
                continue;
            }
        }
        return false;
    }

    return last->children.erase(parts.back()) != 0;
}

std::shared_ptr<std::istream> VTree::Get(const std::string_view path)
{
    if (path.empty())
    {
        return nullptr;
    }
    const std::vector<std::string> parts = Lud::Split(path, FMAN_PREFERRED_SEPARATOR);

    Node* last = &m_root;
    for (const auto& part : parts | std::views::take(parts.size() - 1))
    {
        if (const auto it = last->children.find(part); it != last->children.end())
        {
            if (last = std::get_if<Node>(&it->second); last != nullptr)
            {
                continue;
            }
        }
        return nullptr;
    }
    const auto it = last->children.find(parts.back());
    if (it == last->children.end())
    {
        return nullptr;
    }
    const auto file = std::get_if<VFile>(&it->second);
    if (file == nullptr)
    {
        return nullptr;
    }
    return std::make_shared<Lud::memory_istream<uint8_t>>(file->data);
}

VTree::VFile::VFile(std::vector<uint8_t>&& vec_data)
    : data(std::move(vec_data))
{
}

} // namespace Fman
