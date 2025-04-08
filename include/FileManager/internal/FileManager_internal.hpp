#ifndef FILE_MANAGER_INTERNAL_HEADER
#define FILE_MANAGER_INTERNAL_HEADER

#include <deque>
#include <filesystem>
#include <fstream>
#include <vector>
#include <unordered_map>

#include "MemoryFile.hpp"

namespace Fman::_detail_
{

class Context
{
public:
	Context();
	~Context();

	Context(const Context& other)            = delete;
	Context& operator=(const Context& other) = delete;

	Context(Context&& other)            = delete;
	Context& operator=(Context&& other) = delete;

private:
public:
	std::deque<std::filesystem::path> folders;
	std::filesystem::path root;
	std::filesystem::path current_folder;
	std::fstream current_file;

	std::vector<char*> allocations;

	std::string serialize_filename="srl.dat";

	std::unordered_map<std::string, std::filesystem::path> known_paths;

	// std::mutex m_current_file_mutex;
};




}

namespace Fman::vfs::_detail_ {
class Vfs
{
public:
	Vfs(const Vfs& other)            = delete;
	Vfs& operator=(const Vfs& other) = delete;

	Vfs(Vfs&& other)            = delete;
	Vfs& operator=(Vfs&& other) = delete;
private:
	std::unordered_map<std::string_view, Vfs> m_nodes;
	File* m_data;
};
}

#endif