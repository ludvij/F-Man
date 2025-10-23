#include "internal/pch.hpp"

#include "vfs/MemoryFile.hpp"


using namespace Fman;

vfs::File& vfs::File::operator=(const File& other)
{
	this->m_data = other.m_data;
	this->m_stream.Link(m_data);
	this->m_name = other.m_name;

	return *this;
}

vfs::File& vfs::File::operator=(File&& other)
{
	this->m_data = std::move(other.m_data);
	this->m_stream.Link(m_data);
	this->m_name = std::move(other.m_name);

	return *this;
}

void Fman::vfs::File::Rename(const std::string_view new_name) noexcept
{
	m_name = new_name;
}

