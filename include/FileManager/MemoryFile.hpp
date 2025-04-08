#ifndef FILEMANAGER_MEMORYFILE_HEADER
#define FILEMANAGER_MEMORYFILE_HEADER
#include <optional>
#include <vector>
#include <cstdint>
#include <functional>
namespace Fman::vfs
{
	// should this be movable
// should this be copyable
// should this own its data
// should this just be a record
// should this have functionality inside the class
// should this have functionality outside the class using manipulators
// should this be a struct
class File
{
public:
	using Ref = const std::reference_wrapper<File>;
private:
	using ref_type = File&;
	using val_type = File;
	using ptr_type = File*;
public:


private:
	std::vector<uint8_t> m_data;
	// what flags should we have
	// should we use a u16
	// should we use a struct
	// should we add an enum
	uint16_t m_flags;
};
}
#endif//!FILEMANAGER_MEMORYFILE_HEADER