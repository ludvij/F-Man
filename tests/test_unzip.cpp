#include "FileManager/compression/archive/zip.hpp"
#include <catch2/catch_all.hpp>
#include <ludutils/lud_mem_stream.hpp>

#ifdef _WIN32
    #define EXTERNAL_LINKAGE extern
#else
    #define EXTERNAL_LINKAGE extern "C"
#endif

EXTERNAL_LINKAGE unsigned char TEST_ZIP[];
EXTERNAL_LINKAGE unsigned int TEST_ZIP_len;

TEST_CASE("Unzip memory", "[vfs][unzip]")
{
    Lud::memory_istream<uint8_t> stream({TEST_ZIP, TEST_ZIP_len});
    Fman::Compression::ZipArchive archive(stream);
    auto files = archive.GetDirectory();
    SECTION("Correct folder structure")
    {
        REQUIRE(files[0].file_name == "test/A/");
        REQUIRE(files[1].file_name == "test/A/A/");
        REQUIRE(files[2].file_name == "test/A/B.txt");
        REQUIRE(files[3].file_name == "test/B/");
        REQUIRE(files[4].file_name == "test/B/empty.txt");
        REQUIRE(files[5].file_name == "test/C.txt");
    }

    SECTION("Empty file")
    {
        auto empty = archive.Peek(files[4]);
        auto require_block = [&]() {
            REQUIRE(empty.size() == 0);
        };
        REQUIRE_NOTHROW(require_block());
    }

    SECTION("File with content")
    {
        auto this_is_a_text = archive.Peek(files[2]) | std::ranges::to<std::string>();
        auto this_is_a_test = archive.Peek(files[5]) | std::ranges::to<std::string>();
        auto requires_block = [&]() {
            REQUIRE(this_is_a_test == "this is a test");
            REQUIRE(this_is_a_text == "this is a text");
        };
        REQUIRE_NOTHROW(requires_block());
    }
}
