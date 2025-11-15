#include "FileManager/FileManager.hpp"
#include "FileManager/vfs/Vfs.hpp"
#include "catch2/catch_test_macros.hpp"
#include <array>
#include <catch2/catch_all.hpp>
#include <string>
#include <vector>

TEST_CASE("VFS - Add", "[Fman][vfs]")
{
    auto vfs = Fman::VTree::Create();
    SECTION("Add empty")
    {
        REQUIRE(vfs.Add("this/is/a/test"));
        REQUIRE(vfs.Add("this/is/a/tests"));
        REQUIRE(vfs.Add("this/is/a/test/2"));
        REQUIRE(vfs.Add("Could/be/another"));
        REQUIRE(vfs.Add("this/is/another/test"));

        REQUIRE_FALSE(vfs.Add(""));
        REQUIRE_FALSE(vfs.Add("this"));
        REQUIRE_FALSE(vfs.Add("/this/"));
        REQUIRE_FALSE(vfs.Add("this/is/a/test"));
    }

    SECTION("Add some")
    {
        const std::array<uint8_t, 12> some_data{'t', 'h', 'i', 's', ' ', 'i', 's', ' ', 't', 'e', 's', 't'};
        const auto expected = some_data | std::ranges::to<std::string>();

        REQUIRE(vfs.Add("this/is/a/test", some_data));
        REQUIRE(vfs.Add("this/is/a/tests", some_data));
        REQUIRE(vfs.Add("some/test/2", some_data));
        REQUIRE(vfs.Add("new/test", some_data));

        REQUIRE_FALSE(vfs.Add("", some_data));
        REQUIRE_FALSE(vfs.Add("this/is/a/tests", some_data));
        vfs.Add("some/test");
        REQUIRE_FALSE(vfs.Add("some/test", some_data));
        REQUIRE_FALSE(vfs.Add("new/test/2", some_data));
        REQUIRE_FALSE(vfs.Add("new/test/2"));

        REQUIRE(vfs.Get("") == nullptr);
        REQUIRE(vfs.Get("this/is") == nullptr);
        REQUIRE(vfs.Get("this/is/a/test/2") == nullptr);

        REQUIRE(vfs.Get("this/is/a/test") != nullptr);
        REQUIRE(vfs.Get("this/is/a/tests") != nullptr);
        REQUIRE(vfs.Get("some/test/2") != nullptr);
        REQUIRE(vfs.Get("new/test") != nullptr);

        auto file = vfs.Get("this/is/a/test");

        REQUIRE(Fman::Slurp(*file) == expected);
    }

    SECTION("Add moves")
    {
        std::vector<uint8_t> to_move{'t', 'h', 'i', 's', ' ', 'i', 's', ' ', 't', 'e', 's', 't'};
        const auto expected = to_move | std::ranges::to<std::string>();

        REQUIRE(vfs.Add("this/is/a/test", std::move(to_move)));

        REQUIRE(vfs.Get("this/is/a/test") != nullptr);

        auto file = vfs.Get("this/is/a/test");

        REQUIRE(Fman::Slurp(*file) == expected);
    }
}

TEST_CASE("VFS - Contains", "[Fman][vfs]")
{
    auto vfs = Fman::VTree::Create();
    SECTION("Empty contains")
    {
        vfs.Add("this/is/a/test");

        REQUIRE(vfs.Contains("this"));
        REQUIRE(vfs.Contains("/this/"));
        REQUIRE(vfs.Contains("this/is/a"));
        REQUIRE(vfs.Contains("this/is/a/test"));

        REQUIRE_FALSE(vfs.Contains(""));
        REQUIRE_FALSE(vfs.Contains("thiss"));

        vfs.Add("this/is/another/test");

        REQUIRE(vfs.Contains("this/is/a/test"));
        REQUIRE(vfs.Contains("this/is/another/test"));
    }
}

TEST_CASE("VFS - Remove", "[Fman][vfs]")
{
    auto vfs = Fman::VTree::Create();
    SECTION("Remove empty")
    {
        vfs.Add("this/is/a/test");
        vfs.Add("this/is/a/mock");
        vfs.Add("this/is/a/test/2");

        REQUIRE(vfs.Remove("this/is/a/test"));
        REQUIRE_FALSE(vfs.Remove("this/is/a/test"));
        REQUIRE_FALSE(vfs.Remove("this/is/a/test/2"));

        REQUIRE(vfs.Contains("this/is/a"));
        REQUIRE(vfs.Contains("this/is/a/mock"));
        REQUIRE_FALSE(vfs.Contains("this/is/a/test"));
        REQUIRE_FALSE(vfs.Contains("this/is/a/test/2"));

        REQUIRE(vfs.Remove("this"));
    }

    SECTION("Remode data")
    {
        std::array<uint8_t, 12> some_data{'t', 'h', 'i', 's', ' ', 'i', 's', ' ', 't', 'e', 's', 't'};
        vfs.Add("this/is/a/mock", some_data);
        vfs.Add("this/is/a/test/2", some_data);

        REQUIRE(vfs.Remove("this/is/a/test"));
        REQUIRE(vfs.Remove("this/is/a/mock"));
        REQUIRE_FALSE(vfs.Remove("this/is/a/test"));
        REQUIRE_FALSE(vfs.Remove("this/is/a/test/2"));

        REQUIRE(vfs.Contains("this/is/a"));
        REQUIRE(vfs.Get("this/is/a/mock") == nullptr);
        REQUIRE(vfs.Get("this/is/a/test/2") == nullptr);

        REQUIRE(vfs.Remove("this"));
    }
}
