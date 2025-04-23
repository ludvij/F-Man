#ifndef FILE_MANAGER_MEMORYFS_HEADER
#define FILE_MANAGER_MEMORYFS_HEADER
#include "MemoryFile.hpp"
#include <filesystem>
#include <optional>
#include <cstdint>

#include "ludutils/lud_mem_stream.hpp"

namespace Fman
{
namespace vfs
{


// should we use path for querying
// should we use string view
using query_type = const std::filesystem::path&;
// should these append to current vfs
// should these replace current vfs
bool ReadFromZip(query_type path);
bool ReadFromZip(uint8_t* data, size_t sz);
bool ReadFromPath(query_type path);

// ideally I would have only a Push function with a default empty data
// but, then I did not have a way to differenciate between empty files and folders
bool PushDirectory(query_type path);
bool PushFile(query_type path, Lud::BinaryRange auto&  data);
bool PushFile(query_type path, Lud::BinaryRange auto&& data);


/**
 * @brief removes path from vfs
 * @param path the path to be removed, the matched branch will be culled
 *  so it deletes whole directory structures
 * @return true if path was removed, false if path was not found
 */
bool Pop(query_type path);


// should this return only on file
// should this return all files contained in a directory
// should this return optional
std::optional<File::Ref> Find(query_type path);
File& Get(query_type path);


bool Contains(query_type path);

// should this return optional
// should this work for files and folders
std::optional<size_t> Size(query_type path);


/**
 * @brief deletes whole VFS
 * @return always true
 */
bool Clear();

template<typename T, typename Parser>
std::optional<T> Parse(query_type path);

bool WriteToDisk(query_type root);
// this is brave tbh
bool WriteToZip(query_type name);

}
}



// some template specs


template <typename T, typename Parser>
std::optional<T> Fman::vfs::Parse(query_type path)
{
	Parser parser;
	auto file_opt = Find(path);
	if (!file_opt)
	{
		return std::nullopt;
	}
	auto file = file_opt.value();
	return parser.Parse(file.get());
}


#endif//!FILE_MANAGER_MEMORYFS_HEADER