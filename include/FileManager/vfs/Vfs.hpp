#ifndef FILE_MANAGER_VFS_HEADER
#define FILE_MANAGER_VFS_HEADER

#include <concepts>
#include <cstdint>
#include <filesystem>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

#include <concepts>
#include <format>
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

    /**
     * @brief Gets the full data of a file from a given path
     *
     * @param path The search param
     * @return std::shared_ptr<std::istream> stream to the file
     * @return nullptr if file not found
     */
    std::shared_ptr<std::istream> Get(const std::string_view path);

    /**
     * @brief Adds a path the the tree
     *
     * @param path The path of the file
     * @return true If path was added
     * @return false If path was not be added
     */
    bool Add(const std::string_view path);

    /**
     * @brief Moves a file to the tree given a path
     *
     * @param path The path of the file
     * @param data The data of the file
     * @return true If file was added
     * @return false If file could not be added
     */
    bool Add(const std::string_view path, std::vector<uint8_t>&& data);
    /**
     * @brief Adds a file to the tree given a path
     *
     * @tparam R a generic range of unsigned chars
     * @param path The path of the file
     * @param data The data of the file
     * @return true If the file was added
     * @return false If the file was not added
     */
    template <range_of_char<uint8_t> R>
    bool Add(const std::string_view path, const R& data);

    /**
     * @brief Removes a path from the tree, pruning it in the process
     *        if /1/2/3 is removed, then everything after 3/ will be removed too
     *
     * @param path The path to be removed
     * @return true If the path was removed
     * @return false If the path was not removed
     */
    bool Remove(const std::string_view path);

    /**
     * @brief Checks if the tree contains a path
     *
     * @param path The path to be checked
     * @return true If the tree contains the path
     * @return false If the tree does not contain the path
     */
    bool Contains(const std::string_view path) const;

    /**
     * @brief Inserts all files contained in a zip file to the tree
     *
     * @param stream a stream that points to the zip file
     * @return size_t number of added files
     */
    size_t LoadZip(std::istream& stream);

    /**
     * @brief Inserts all files contained in a path to the tree, does dfs to the path
     *
     * @param path The path to be included
     * @return size_t The number of files added
     */
    size_t LoadFrom(const std::filesystem::path& path);

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

    friend struct std::formatter<VTree>;
};

template <range_of_char<uint8_t> R>
bool VTree::Add(const std::string_view path, const R& data)
{
    auto data_vector = data | std::ranges::to<std::vector>();
    return Add(path, std::move(data_vector));
}

} // namespace Fman

template <>
struct std::formatter<Fman::VTree>
{
    auto format(const Fman::VTree& obj, std::format_context& ctx) const
    {
        const auto dfs_impl = [&ctx](const Fman::VTree::Node& node, size_t depth, auto& impl) -> void
        {
            std::string indentation(depth * 1, ' ');
            for (const auto& [path, elem] : node.children)
            {
                if (std::holds_alternative<Fman::VTree::Node>(elem))
                {
                    std::format_to(ctx.out(), "{}{}/\n", indentation, path);
                    auto& next = std::get<Fman::VTree::Node>(elem);
                    impl(next, depth + 1, impl);
                }
                else
                {
                    size_t size = std::get<Fman::VTree::VFile>(elem).data.size();
                    std::format_to(ctx.out(), "{}*{} [{}]\n", indentation, path, size);
                }
            }
        };
        std::format_to(ctx.out(), "/\n");

        dfs_impl(obj.m_root, 1, dfs_impl);

        return ctx.out();
    }

    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
};
#endif // FILE_MANAGER_VFS_HEADER
