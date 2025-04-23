#include "internal/pch.hpp"

#include "vfs/Memoryfs.hpp"
#include "vfs/MemoryFile.hpp"

#include <ludutils/lud_archive.hpp>
#include <ludutils/lud_mem_stream.hpp>
#include <ludutils/lud_parse.hpp>


using Fman::vfs::_detail_::Vfs;

Vfs root;

Vfs* internal_push_path(std::forward_iterator auto first, std::forward_iterator auto last)
{
	Vfs* trie = &root;
	for(auto& it = first; it != last; ++it)
	{
		trie = &trie->nodes[*it];
	}
	return trie;
}

Vfs* internal_search(std::forward_iterator auto first, std::forward_iterator auto last)
{
	Vfs* trie = &root;
	for(auto& it = first; it != last; ++it)
	{
		if (!trie->nodes.contains(*it))
		{
			return nullptr;
		}
		trie = &trie->nodes.at(*it);
	}

	return trie;
}

bool Fman::vfs::ReadFromZip(uint8_t *data, size_t sz)
{
	Lud::memory_istream<uint8_t> mem_stream(data, sz);
	auto zip_dir = Lud::CreateZipDirectory(mem_stream);
	for(const auto& zipped_file : zip_dir)
	{
		std::filesystem::path path(zipped_file.file_name);
		bool res;
		if (path.has_filename())
		{
			auto raw_data = Lud::UncompressDeflateStream(zipped_file, mem_stream);
			res = PushFile(path, std::move(raw_data));
		}
		else
		{
			res = PushDirectory(path);
		}
		if (res == false)
		{
			Clear();
			return false;
		}
	}
	return true;
}


bool Fman::vfs::PushDirectory(query_type path) 
{
	auto parts = Lud::Split(path.generic_string(), "/");
	internal_push_path(parts.begin(), parts.end());
	return true;
}

bool Fman::vfs::PushFile(query_type path, Lud::BinaryRange auto& data)
{
	if (!path.has_filename())
	{
		return false;
	}
	auto parts = Lud::Split(path.generic_string(), "/");

	Vfs* trie = internal_push_path(parts.begin(), parts.end());

	trie->file.Rename(parts.back());
	trie->file.SetData(data);

	return true;
}

bool Fman::vfs::PushFile(query_type path, Lud::BinaryRange auto&& data)
{
	if (!path.has_filename())
	{
		return false;
	}
	std::vector<std::string> parts = Lud::Split(path.generic_string(), "/");

	Vfs* trie = internal_push_path(parts.begin(), parts.end());


	trie->file.Rename(parts.back());
	trie->file.SetData(std::move(data));

	return true;
}

bool Fman::vfs::Pop(query_type path)
{
	if (path == "/")
	{
		return Clear();
	}
	else
	{
		auto parts = Lud::Split(path.generic_string(), "/");
		Vfs* trie = internal_search(parts.begin(), parts.end());
		if (trie == nullptr)
		{
			return false;
		}
		trie->Clear();
	}

	return true;
}

bool Fman::vfs::Clear()
{
	root.Clear();
	return true;
}