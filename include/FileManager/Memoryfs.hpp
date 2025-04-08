#ifndef FILE_MANAGER_MEMORYFS_HEADER
#define FILE_MANAGER_MEMORYFS_HEADER
#include "FileManager.hpp"
#include "MemoryFile.hpp"
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

bool PushFolder(query_type path);
bool PushFile(query_type path, std::span<uint8_t> data);
// should we use the same function for files and folders
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

// should be same as popping root
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