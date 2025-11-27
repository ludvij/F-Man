#ifndef VARF_HEADER
#define VARF_HEADER

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace varf {

namespace mode {

constexpr int read = std::ios::in;
constexpr int write = std::ios::out;
constexpr int end = std::ios::ate;
constexpr int append = std::ios::app;
constexpr int binary = std::ios::binary;

} // namespace mode

namespace traverse {

constexpr uint8_t files = 0x01;
constexpr uint8_t folders = 0x02;
constexpr uint8_t all = 0xFF;

} // namespace traverse

/**
 * @brief represents max depth for traversal
 *
 */
constexpr int TRAVERSAL_FULL = -1;
constexpr size_t POP_FULL = std::numeric_limits<size_t>::max();

using OpenMode = int;
using TraverseMode = uint8_t;

struct TraverseOptions
{
    int depth = TRAVERSAL_FULL;
    TraverseMode mode = traverse::all;
    std::vector<std::string_view> filters{};
};

template <typename T>
concept GrowableContiguosRange = requires(T range) {
    { std::ranges::contiguous_range<T> };
    { sizeof(std::ranges::range_value_t<T>) == 1 };
    { range.data() } -> std::same_as<std::ranges::range_value_t<T>*>;
    { range.resize(size_t()) } -> std::same_as<void>;
    { range.reserve(size_t()) } -> std::same_as<void>;
};

/**
 * @brief Get the Current pushed path
 *
 * @return std::filesystem::path
 */
std::filesystem::path GetCurrent();
/**
 * @brief Get the Current root path
 *
 * @return std::filesystem::path
 */
std::filesystem::path GetRoot();

/**
 * @brief Set the Root object
 *
 * @param path (optional) name of the new root
 *             if empty will set root to current
 *             same rules as @ref Push except path can be absolute
 *
 * @throws std::runtime_error if the path is not a valid path
 * @throws std::filesystem::filesystem_error if path does not exist and creation fails
 * @throws std::filesystem::filesystem_error if setting current working directory fails
 *         if this error happens the varf context will be reset
 *
 * @throws std::runtiem_error if folder could not be created
 */
void SetRoot(const std::filesystem::path& path = {});

/**
 * @brief Set the root to a known path
 *
 * @param name the name of the knonw path
 */
void SetRootToKnownPath(const std::string& name);

/**
 * @brief resets varfs filemanager
 *
 */
void Reset();

/**
 * @brief Pushes a new folder on top of current
 *
 * @param name new folder name, will be appended to the current one
 *             if multiple paths for/example it will be pushed as two
 *             a valid pathname
 *                 will not contain \?%*:|"<>,;=
 *                 will not end in .
 *                 will not be an absolute path
 *                 and will not point to anything that is not a directory
 *             if you push valid/error. even an exception will be thrown
 *
 * @param create true if create folder if not exists

 * @throws std::runtime_error if the path is not a valid path
 * @throws std::filesystem::filesystem_error if you try to push("..") in root
 * @throws std::filesystem::filesystem_error if creation fails on create == true
 * @throws std::filesystem::filesystem_error if setting current working directory fails
 *         if this error happens the varf context will be reset
 *
 * @return true if folder was entered
 *              or name was empty
 * @return false if create = false and directory does not exist
 */
bool Push(const std::filesystem::path& name, bool create = true);

/**
 * @brief goes back to previous folder
 *
 * @param amount the number of folders to go back
 *               if POP_FULL then all folders will be popped
 *
 * @throws std::runtime_error if you try to pop while on root
 * @throws std::runtime_error if amount > number of pushed directories
 * @throws std::filesystem::filesystem_error if setting current working directory fails
 *         if this error happens the varf context will be reset
 */
void Pop(size_t amount = 1);

/**
 * @brief Opens a file
 *
 * @param name name of the file to be created
 * @param mode open mode of file
 *
 * @return ptr to file stream of opened file if file is opened or nullptr in case of error
 */
[[nodiscard]]
std::shared_ptr<std::fstream> PushFile(const std::filesystem::path& name, OpenMode mode = mode::read);

/**
 * @brief Traverses current directory and retrieves all files up to given depth
 *
 * @param options options of the traversal
 *                depth is the amount of subdirectories to traverse
 *                    set to TRAVERSAL_FULL to traverse until exhaustion
 *                mode is the traversal mode, can be files, folders or either
 *                filters only applies when mode contains files
 *                    only returns files that match the extension filter
 *
 * @throws std::runtime_error if options.depth == 0
 *
 * @return std::vector<std::filesystem::path>  containing all traversables up to specified depth
 */
[[nodiscard]]
std::vector<std::filesystem::path> Traverse(const TraverseOptions& options = {});

/**
 * @brief returns whole file from current stream postion to end
 *
 * @tparam GrowableContigousRange a growable contigous range
 * @param stream the stream of the file
 * @return std::string containg file
 */
template <GrowableContiguosRange Rng = std::string>
[[nodiscard]]
Rng Slurp(std::istream& stream)
{
    const std::streamsize current_pos = stream.tellg();
    stream.seekg(0, std::ios::end);
    const std::streamsize size = stream.tellg();
    stream.seekg(current_pos, std::ios::beg);

    Rng res;
    res.resize(size);

    stream.read(reinterpret_cast<char*>(res.data()), size);

    return res;
}

/**

 * @brief returns whole file as a single string
 *
 * @tparam GrowableContigousRange a growable contigous range
 * @param path the path to the file
 * @return string containing file
 */
template <GrowableContiguosRange Rng = std::string>
[[nodiscard]]
Rng Slurp(const std::filesystem::path& path)
{
    auto file = PushFile(path, mode::read | mode::end);
    return Slurp<Rng>(*file);
}

namespace rcs {
/**
 * @brief Obtains a stream to a resourece
 *
 * @param path The path of the resource
 * @return std::shared_ptr<std::istream> smart pointer containing file
 */
[[nodiscard]]
std::shared_ptr<std::istream> Get(const std::string_view path);

/**
 * @brief Obtains a resource as a string
 *
 * @tparam GrowableContigousRange a growable contigous range
 * @param path the path of the resource
 * @return std::string the resource as a string
 */
template <GrowableContiguosRange Rng = std::string>
[[nodiscard]]
Rng Slurp(const std::string_view path)
{
    auto file = rcs::Get(path);

    return varf::Slurp<Rng>(*file);
}

} // namespace rcs

} // namespace varf
#endif
