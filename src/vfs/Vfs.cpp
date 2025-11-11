#include "FileManager/vfs/Vfs.hpp"

#include <ludutils/lud_parse.hpp>
#include <variant>

namespace Fman
{

VTree VTree::Create() { return {}; }

bool VTree::Add(const std::string_view path)
{
    const auto parts = Lud::Split(path, "/");
    Node* last = &m_root;
    for (const auto& part : parts)
    {
        auto variant = last->children[part];
        if (auto node = std::get_if<Node>(&variant); node)
        {
            last = node;
        }
        else
        {
            return false;
        }
    }
    return true;
}

bool VTree::Add(const std::string_view path, std::vector<uint8_t>&& data)
{
    const auto parts = Lud::Split(path, "/");
    Node* last = &m_root;
    for (const auto& part : parts | std::views::take(parts.size() - 1))
    {
        auto variant = last->children[part];
        if (auto node = std::get_if<Node>(&variant); node)
        {
            last = node;
        }
        else
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
    const auto parts = Lud::Split(path, "/");
    Node* last = const_cast<Node*>(&m_root);
    for (const auto& part : parts)
    {
        if (!last->children.contains(part))
        {
            return false;
        }
        if (auto node = std::get_if<Node>(&last->children[part]); node)
        {
            last = node;
        }
        else
        {
            return false;
        }
    }
    return true;
}

VTree::VFile::VFile(std::vector<uint8_t>&& vec_data) : data(std::move(vec_data))
{
}

} // namespace Fman
