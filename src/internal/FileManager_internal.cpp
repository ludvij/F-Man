#include "internal/pch.hpp"

#include "internal/FileManager_internal.hpp"

#if defined(FILEMANAGER_PLATFORM_WINDOWS)
#include <ShlObj.h>
#include "FileManager_internal.hpp"
Fman::_detail_::Context::Context()
{
	// I need to clan this
	auto add_knonw_path = [&](const std::string& name, GUID id)
		{
			PWSTR path;

			auto err = SHGetKnownFolderPath(id, KF_FLAG_CREATE, NULL, &path);
			if (err != S_OK)
			{
				CoTaskMemFree(path);
				throw std::runtime_error("path not found");
			}
			known_paths.emplace(name, path);

			CoTaskMemFree(path);
		};
	add_knonw_path("HOME", FOLDERID_Profile);
	add_knonw_path("APPDATA", FOLDERID_RoamingAppData);
	add_knonw_path("DOCUMENTS", FOLDERID_Documents);
	known_paths.emplace("PWD", std::filesystem::current_path());

	root = current_folder = known_paths.at("PWD");
}
#elif defined(FILEMANAGER_PLATFORM_LINUX)
Fman::_detail_::Context::Context()
{

	known_paths.emplace("HOME", "~");
	known_paths.emplace("APPDATA", "~");
	known_paths.emplace("DOCUMENTS", "~/Documents");

	known_paths.emplace("PWD", std::filesystem::current_path());

	root = current_folder = known_paths.at("PWD");
}
#else
#error Platform not supported
#endif


Fman::_detail_::Context::~Context()
{
	for (const auto& alloc : allocations)
	{
		delete[] alloc;
	}
}
// TODO: make this not recursive
size_t Fman::vfs::_detail_::Vfs::Size() const
{
	size_t self_size = nodes.size();
	size_t nodes_size = 0;
	for(const auto& [path, node] : nodes)
	{
		nodes_size += node.Size();
	}

	return self_size + nodes_size;
}

void Fman::vfs::_detail_::Vfs::Clear()
{
	nodes.clear();
	file = File();
}
