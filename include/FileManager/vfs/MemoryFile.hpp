#ifndef FILEMANAGER_MEMORYFILE_HEADER
#define FILEMANAGER_MEMORYFILE_HEADER
#include <optional>
#include <vector>
#include <cstdint>
#include <functional>
#include <string_view>
#include <string>

#include "ludutils/lud_mem_stream.hpp"


namespace Fman::vfs
{
// should this just be a record
class File
{
public:
	using Ref = const std::reference_wrapper<File>;
private:
public:

	File() = default;
	File(const std::string_view name, const Lud::BinaryRange auto&  range); // reference collapsing being a pain
	File(const std::string_view name,       Lud::BinaryRange auto&  range); // reference collapsing being a pain
	File(const std::string_view name,       Lud::BinaryRange auto&& range);

	File& operator=(const File& other);
	File& operator=(File&& other);
	
	void Rename(const std::string_view new_name) noexcept;
	std::string Name() const { return m_name;}

	void SetData(const Lud::BinaryRange auto&  range);
	void SetData(      Lud::BinaryRange auto&  range);
	void SetData(      Lud::BinaryRange auto&& range);

	std::istream& Stream() { return m_stream; }

	bool IsInit() const { return m_init; }

private:
	std::vector<uint8_t> m_data;
	// should the file own the data, or the vfs
	std::string m_name;
	// what flags should we have
	// should we use a u16
	// should we use a struct
	// should we add an enum
	uint16_t m_flags;
	Lud::memory_istream<uint8_t> m_stream;

	bool m_init = false;
};



void File::SetData(const Lud::BinaryRange auto &range)
{
	m_data.clear();
	m_data.reserve(range.size());
	std::copy(range.begin(), range.end(), std::back_inserter(m_data));

	m_stream.Link(m_data);
	m_init = true;

}

void File::SetData(Lud::BinaryRange auto& range)
{
	m_data.clear();
	m_data.reserve(range.size());
	std::copy(range.begin(), range.end(), std::back_inserter(m_data));

	m_stream.Link(m_data);
	m_init = true;

}

void File::SetData(Lud::BinaryRange auto&& range)
{
	m_data.clear();
	m_data.insert(m_data.end(), std::make_move_iterator(range.begin()), std::make_move_iterator(range.end()));

	m_stream.Link(m_data);
	m_init = true;
}

File::File(const std::string_view name, Lud::BinaryRange auto &&range)
	: m_name(name)
{
	SetData(std::move(range));
}


File::File(const std::string_view name, const Lud::BinaryRange auto& range)
	: m_name(name)
{
	SetData(range);
}

File::File(const std::string_view name, Lud::BinaryRange auto& range)
	: m_name(name)
{
	SetData(range);
}


}
#endif//!FILEMANAGER_MEMORYFILE_HEADER