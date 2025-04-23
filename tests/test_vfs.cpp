#include <catch2/catch_all.hpp>
#include "FileManager/vfs/Memoryfs.hpp"
#include "FileManager/internal/FileManager_internal.hpp"

#include <print>

#ifdef _WIN32
#define EXTERNAL_LINKAGE_C extern
#define EXTERNAL_LINKAGE_CPP extern
#else
#define EXTERNAL_LINKAGE_C extern "C"
#define EXTERNAL_LINKAGE_CPP extern "C++"
#endif

EXTERNAL_LINKAGE_C unsigned char TEST_BINDUMP[];
EXTERNAL_LINKAGE_C unsigned int TEST_BINDUMP_len;

EXTERNAL_LINKAGE_CPP Fman::vfs::_detail_::Vfs root;




TEST_CASE("VSF -- Read", "[Fman][vfs]")
{
	root.Clear();
	SECTION("Read from path")
	{
		Fman::vfs::ReadFromZip(TEST_BINDUMP, TEST_BINDUMP_len);
		
		REQUIRE(root.Size() == 9);
		REQUIRE(root.nodes.contains("test"));
		auto trie_test = &root.nodes.at("test");
			REQUIRE(trie_test->nodes.contains("1"));
			auto trie_1 = &trie_test->nodes.at("1");
				REQUIRE(trie_1->nodes.contains("A"));
				auto trie_1_a = &trie_1->nodes.at("A");
					REQUIRE(trie_1_a->nodes.contains("B.txt"));
					auto trie_1_a_btxt = &trie_1_a->nodes.at("B.txt");
					REQUIRE(trie_1_a_btxt->file.IsInit() == true);
				REQUIRE(trie_1->nodes.contains("B.txt"));
			REQUIRE(trie_test->nodes.contains("2"));
			auto trie_2 = &trie_test->nodes.at("2");
			REQUIRE(trie_2->file.IsInit() == false);
			REQUIRE(trie_test->nodes.contains("3"));
			auto trie_3 = &trie_test->nodes.at("3");
				REQUIRE(trie_3->nodes.contains(".dotfile"));
				REQUIRE(trie_3->nodes.contains("noext"));
	}

	SECTION("Read from zip")
	{

	}
}

TEST_CASE("VSF -- Write", "[Fman][vfs]")
{
	SECTION("Write to disk")
	{

	}

	SECTION("write to zip")
	{

	}
}


TEST_CASE("VFS -- Add file", "[Fman][vfs][file]")
{
	SECTION("Push")
	{

	}

	SECTION("Push multiple")
	{

	}

	SECTION("Push multiple with same name")
	{

	}

	SECTION("Push multiple with same name different path")
	{

	}

	SECTION("Invalide name")
	{

	}

	SECTION("Empty name")
	{
		
	}
}

TEST_CASE("VFS -- Add folder", "[Fman][vfs][folder]")
{
	SECTION("Push")
	{

	}

	SECTION("Push multiple")
	{

	}

	SECTION("Push multiple with same name")
	{

	}

	SECTION("Push multiple with same name different path")
	{

	}

	SECTION("Invalid path")
	{

	}

	SECTION("Empty path")
	{

	}
}

TEST_CASE("VFS -- Remove", "[Fman][vfs]")
{
	SECTION("Pop file")
	{

	}

	SECTION("Pop folder")
	{

	}

	SECTION("Pop nonexistant file")
	{

	}

	SECTION("Pop nonexistant folder")
	{

	}

	SECTION("Pop *")
	{

	}

	SECTION("Clear")
	{

	}
}


TEST_CASE("VFS -- Find", "[Fman][vfs]")
{
	SECTION("Find file")
	{

	}

	SECTION("Find folder")
	{

	}

	SECTION("Find nonexistant file")
	{

	}

	SECTION("Find nonexistant folder")
	{

	}
}


TEST_CASE("VFS -- size", "[Fman][vfs]")
{
	SECTION("size of file")
	{

	}

	SECTION("size of folder")
	{

	}

	SECTION("Size of nonexistant file")
	{

	}

	SECTION("Size of nonexistant folder")
	{

	}
}