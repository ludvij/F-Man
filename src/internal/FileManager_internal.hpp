#ifndef FILE_MANAGER_INTERNAL_HEADER
#define FILE_MANAGER_INTERNAL_HEADER


#include <deque>
#include <filesystem>
#include <vector>
#include <unordered_map>


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

	std::string serialize_filename="srl.dat";

	std::filesystem::path root;
	std::filesystem::path current_folder;

	std::unordered_map<std::string, std::filesystem::path> known_paths;
	std::deque<std::filesystem::path> folders;
	std::vector<char*> allocations;
	
	// std::mutex m_current_file_mutex;
};




}
#endif