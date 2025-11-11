#ifndef FILE_MANAGER_HEADER
#define FILE_MANAGER_HEADER

#include <filesystem>
#include <iostream>
#include <optional>
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

bool SetRootToKnownPath(const std::string& name);

/**
 * @brief Pushes a nes folder on top of current
 *
 * @param name new folder name, will be appended to the current one
 * @param create true if create folder if not exists
 *
 * @return true if folder was entered
 * @return false if folder was not entered
 */
bool PushFolder(const std::filesystem::path& name, bool create = true);

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
 * @return file stream of opened file if file is opened or std::nullopt in case of fail
 */
std::optional<std::fstream> PushFile(const std::filesystem::path& name, OpenMode mode = mode::write | mode::append);

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
 * @param path the path to the file
 * @return string containing file contents
 */
std::string Slurp(const std::filesystem::path& path);

} // namespace Fman
#endif
