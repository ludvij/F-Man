#ifndef FILE_MANAGER_VFS_HEADER
#define FILE_MANAGER_VFS_HEADER

#include <concepts>
#include <cstdint>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

#include <concepts>
#include <variant>

namespace Fman
{

template <typename R, typename V>
concept range_of_char = requires(R r) {
    requires std::ranges::range<R>;
    requires sizeof(V) == 1;
    requires std::same_as<std::ranges::range_value_t<R>, V>;
};

class VTree
{
public:
    VTree(const VTree&) = delete;
    VTree& operator=(const VTree&) = delete;
    VTree(VTree&&) = delete;
    VTree& operator=(VTree&&) = delete;

    static VTree Create();

    bool Add(const std::string_view path);

    bool Add(const std::string_view path, std::vector<uint8_t>&& data);
    template <range_of_char<uint8_t> R>
    bool Add(const std::string_view path, const R& data);

    bool Contains(const std::string_view path) const;

private:
    VTree() = default;

private:
    struct VFile
    {
        VFile(std::vector<uint8_t>&& data);

        std::vector<uint8_t> data;
    };
    struct Node
    {
        std::unordered_map<std::string, std::variant<Node, VFile>> children;
    };

    Node m_root;
};

template <range_of_char<uint8_t> R>
bool VTree::Add(const std::string_view path, const R& data)
{
    auto data_vector = data | std::ranges::to<std::vector>();
    return Add(path, std::move(data_vector));
}

} // namespace Fman

#endif // FILE_MANAGER_VFS_HEADER
