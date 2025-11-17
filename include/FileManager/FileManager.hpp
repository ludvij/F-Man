#ifndef FILE_MANAGER_HEADER
#define FILE_MANAGER_HEADER

#include <filesystem>
#include <fstream>
#include <iostream>
#include <ludutils/lud_assert.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Fman
{

namespace mode
{

constexpr int read = std::ios::in;
constexpr int write = std::ios::out;
constexpr int end = std::ios::ate;
constexpr int append = std::ios::app;
constexpr int binary = std::ios::binary;

} // namespace mode

namespace traverse
{

constexpr uint8_t files = 0x01;
constexpr uint8_t folders = 0x02;
constexpr uint8_t all = 0xFF;

} // namespace traverse

constexpr int FULL = -1;

using OpenMode = int;
using TraverseMode = uint8_t;

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
 *             if not set will set root to current
 *
 * @return true if folder was created
 * @return false if folder exists
 */
bool SetRoot(const std::filesystem::path& path = {});

/**
 * @brief Set the root to a known path
 *
 * @param name the name of the knonw path
 * @return true If the root was set correctly
 * @return false If the root was not set correctly
 */
bool SetRootToKnownPath(const std::string& name);

/**
 * @brief Pushes a new folder on top of current
 *
 * @param name new folder name, will be appended to the current one
 * @param create true if create folder if not exists
 *
 * @return true if folder was entered
 * @return false if folder was not entered
 */
bool PushFolder(const std::filesystem::path& name, bool create = true);

/**
 * @brief Returns a path appended to the current path
 *
 * @param path The path to append
 * @return std::filesystem::path resulting path
 */
std::filesystem::path GetFromCurrent(const std::filesystem::path& path);

/**
 * @brief Pushes multiple folders
 * @param names list of folders
 * @param create true will create folder if it does not exists
 * @return true if all folders entered successfully
 * @return false if all folders were not entered
 */
bool PushFolder(std::initializer_list<std::filesystem::path> names, bool create = true);

/**
 * @brief Allocates and returns file name in current
 *        The user does not need to delete the allocated memory
 *        Dirty little hack for imgui
 *
 * @param name name of file
 *
 * @return cstring containing current_path/name
 */
char* AllocateFileName(const char* name);

/**
 * @brief goes back to previous folder
 *
 * @param amount the number of folders to go back
 *               if < 0 then all folders will be popped
 */
void PopFolder(int amount = 1);

/**
 * @brief Opens a file
 *
 * @param name name of the file to be created
 * @param mode open mode of file
 *
 * @return ptr to file stream of opened file if file is opened or std::nullopt in case of fail
 */
std::shared_ptr<std::fstream> PushFile(const std::filesystem::path& name, OpenMode mode = mode::write | mode::append);

/** A
 * @fn Traverse
 * @brief Traverses current directory and retrieves all files up to given depth
 *
 * @param depth the depth of the search
 * @param trav_mode the type of traverse Fman::traverse::all by default
 *  - Fman::traverse::files for only files
 *  - Fman::traverse::folders for only folders
 *  - Fman::traverse::files | Fman::Traverse::folders for files and folders
 *  - Fman::traverse::all for all
 * @param filters file extensions, if empty filtering is off, if not empty will only return
 *  files with extensions in filers
 *
 * @return std::vector<std::filesystem::path>  containing all traversables up to specified depth
 */
std::vector<std::filesystem::path> Traverse(int depth = 1, TraverseMode trav_mode = traverse::all, std::initializer_list<std::string_view> filters = {});

/**
 * @brief returns whole file as a single string
 *
 * @tparam GrowableContigousRange a growable contigous range
 * @param stream the stream of the file
 * @return std::string containg file
 */
template <GrowableContiguosRange Rng = std::string>
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
Rng Slurp(const std::filesystem::path& path)
{
    auto file = PushFile(path, mode::read | mode::end);
    return Slurp<Rng>(*file);
}

namespace Resources
{
/**
 * @brief Obtains a stream to a resourece
 *
 * @param path The path of the resource
 * @return std::shared_ptr<std::istream> smart pointer containing file
 */
std::shared_ptr<std::istream> Push(const std::string_view path);

/**
 * @brief Obtains a resource as a string
 *
 * @tparam GrowableContigousRange a growable contigous range
 * @param path the path of the resource
 * @return std::string the resource as a string
 */
template <GrowableContiguosRange Rng = std::string>
Rng Slurp(const std::string_view path)
{
    auto file = Resources::Push(path);

    return Fman::Slurp<Rng>(*file);
}

} // namespace Resources

} // namespace Fman
#endif
