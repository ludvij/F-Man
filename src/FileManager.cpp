#include "internal/pch.hpp"

#include "FileManager.hpp"
#include "Serializable.hpp"

#include <ludutils/lud_assert.hpp>
#include <ludutils/lud_archive.hpp>

#include "util/compression_utils.hpp"

namespace fs = std::filesystem;
namespace ranges = std::ranges;

Fman::_detail_::Context context;

fs::path Fman::GetCurrent()
{
	return context.current_folder;
}

fs::path Fman::GetRoot()
{
	return context.root;
}

bool Fman::SetRoot(const fs::path& name)
{
	if (name.empty())
	{
		context.root = context.current_folder;
		context.folders.clear();
		return false;
	}
	else
	{
		context.root = context.current_folder = name;
		context.folders.clear();

		return fs::create_directories(context.root);
	}
}

bool Fman::SetRootToKnownPath(const std::string& name)
{
	if (auto path = context.known_paths.find(name); path != context.known_paths.end())
	{
		return SetRoot(path->second);
	}
	return false;
}


bool Fman::PushFolder(const fs::path& name, bool create/*= true*/)
{
	Lud::assert::that(!context.current_file.is_open(), "Can't push folder before popping file");
	Lud::assert::that(!name.has_extension(), "Can't push file while pushing folder");

	if (create)
	{
		context.current_folder /= name;
		context.folders.emplace_back(fs::absolute(context.current_folder));

		fs::create_directories(context.current_folder);
		return true;
	}
	else
	{
		const bool exists = fs::exists(name);
		if (!exists)
		{
			return false;
		}
		context.folders.emplace_back(name);
		context.current_folder /= name;

		return true;
	}

}

bool Fman::PushFolder(std::initializer_list<fs::path> name, bool create/*= true*/)
{
	uint32_t res = false;
	for (const auto& f : name)
	{
		res += PushFolder(f, create);
	}
	return res == name.size();
}

char* Fman::AllocateFileName(const char* name)
{
	auto current = GetCurrent();
	current.append(name);
	auto repr = current.string();
	size_t sz = repr.size() + 1;

	char* s = new char[sz]
		{
			0
		};
		if (s)
		{
		#if defined(__STDC_LIB_EXT1__) && defined(__STDC_WANT_LIB_EXT1__) && __STDC_WANT_LIB_EXT1__ == 1
			std::strncpy_s(s, sz, repr.c_str(), sz);
		#else
			std::strncpy(s, repr.c_str(), sz);
		#endif
		}
		context.allocations.push_back(s);

		return s;
}

void Fman::PopFolder(int amount)
{
	size_t u_amount = std::abs(amount);
	Lud::assert::leq(u_amount, context.folders.size(), "Can't pop more folders than pushed amount");
	Lud::assert::that(!context.current_file.is_open(), "Can't pop folder before popping file");

	if (amount < 0 || u_amount == context.folders.size())
	{
		context.folders.clear();
		context.folders.push_back(context.root);
	}
	else
	{
		context.folders.resize(context.folders.size() - amount);
	}
	context.current_folder = context.folders.back();
}

bool Fman::PushFile(fs::path name, OpenMode mode)
{
	Lud::assert::that(!context.current_file.is_open(), "You need to call PopFile before calling PushFile again");

	fs::path path = context.current_folder;
	path.append(name.string());
	if (!fs::exists(path) && ( mode & mode::read ))
	{
		return false;
	}
	context.current_file.open(path, static_cast<std::ios_base::openmode>( mode ));

	return context.current_file.is_open();
}

void Fman::PopFile()
{
	Lud::assert::that(context.current_file.is_open(), "You need to call PushFile before calling PopFile");

	context.current_file.close();
}

std::vector<std::filesystem::path> Fman::Traverse(const int depth, const TraverseMode trav_mode, const std::initializer_list<std::string_view> filters)
{
	Lud::assert::ne(depth, 0, "depth must be different from 0");
	std::vector<fs::path> result;

	int curr_depth = 0;
	std::deque<fs::directory_entry> iters;

	iters.emplace_back(context.current_folder);

	size_t size = 1;
	while (iters.size() > 0)
	{
		fs::directory_iterator it{ iters.front() };
		iters.pop_front();
		for (const auto& dir_entry : it)
		{
			if (fs::is_directory(dir_entry))
			{
				if (trav_mode & traverse::folders)
					result.emplace_back(dir_entry);
				iters.push_back(dir_entry);
			}
			if (!fs::is_directory(dir_entry) && ( trav_mode & traverse::files ))
			{
				if (filters.size() > 0)
				{
					const auto ext = dir_entry.path().extension();
					if (!( ranges::find(filters, ext) != filters.end() )) continue;
				}
				result.emplace_back(dir_entry);
			}
		}
		if (--size == 0)
		{
			size = iters.size();
			if (depth > FULL && ++curr_depth >= depth)
				return result;
		}
	}
	return result;
}

void Fman::Write(const std::string_view text)
{
	Lud::assert::that(context.current_file.is_open(), "You need to call PushFile before writing to a file");

	context.current_file << text;
}

std::string Fman::Slurp(std::filesystem::path path)
{
	if (!PushFile(path, mode::read | mode::end | mode::binary))
	{
		return {};
	}
	const size_t size = context.current_file.tellg();
	context.current_file.seekg(0, std::ios::beg);

	char* buf = new char[size];
	context.current_file.read(buf, size);
	std::string res(buf, size);

	delete[] buf;
	PopFile();

	return res;

}

void Fman::Serialize(ISerializable* serial)
{
	if (PushFile(context.serialize_filename, mode::binary | mode::write))
	{
		serial->Serialize(context.current_file);
		PopFile();
	}
}

void Fman::Deserialize(ISerializable* serial)
{
	if (PushFile(context.serialize_filename, mode::binary | mode::read))
	{
		serial->Deserialize(context.current_file);
		PopFile();
	}
}

void Fman::SerializeCompress(ISerializable* serial)
{
	std::filesystem::path path = context.current_folder;
	path /= context.serialize_filename;

	Fman::CompressionOfstream comp_ofstream(path);
	serial->Serialize(comp_ofstream);
}

void Fman::DeserializeDecompress(ISerializable* serial)
{
	std::filesystem::path path = context.current_folder;
	path /= context.serialize_filename;

	Fman::DecompressionIfstream comp_ifstream(path);
	serial->Deserialize(comp_ifstream);
}

void Fman::SetSerializeFilename(std::string_view name)
{
	if (!name.empty())
	{
		context.serialize_filename = name;
	}
	else
	{
		context.serialize_filename = "srl.dat";
	}

}

void Fman::SerializeData(std::ostream& strm, const char* data, const size_t sz)
{
	strm.write(data, sz);
}

void Fman::DeserializeData(std::istream& strm, char* data, const size_t sz)
{
	strm.read(data, sz);
}
