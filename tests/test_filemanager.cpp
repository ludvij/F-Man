#include <catch2/catch_all.hpp>
#include <filesystem>
#include <fstream>

#include "FileManager/FileManager.hpp"
#include "catch2/catch_test_macros.hpp"

TEST_CASE("Filemanager - Push", "[varf][filemanager]")
{
    varf::Push("varf_tests/filemanager");
    varf::SetRoot();
    auto pwd = std::filesystem::current_path();

    SECTION("Push single")
    {
        REQUIRE(varf::Push("single"));
        REQUIRE(varf::GetCurrent() == pwd / "single");
        REQUIRE(varf::GetCurrent() != varf::GetRoot());
        REQUIRE_NOTHROW(varf::Pop());
        REQUIRE_THROWS(varf::Pop());
    }

    SECTION("Push multiple")
    {
        REQUIRE(varf::Push("multiple/folders/"));
        REQUIRE(varf::GetCurrent() == pwd / "multiple/folders");
        REQUIRE(varf::GetCurrent() != varf::GetRoot());
        REQUIRE_NOTHROW(varf::Pop());
        REQUIRE_NOTHROW(varf::Pop());
        REQUIRE_THROWS(varf::Pop());
    }

    SECTION("Push after set root")
    {
        REQUIRE(varf::Push("changed root"));
        REQUIRE(varf::GetCurrent() != varf::GetRoot());
        varf::SetRoot();
        REQUIRE(varf::GetCurrent() == varf::GetRoot());
        REQUIRE(varf::Push("whatever"));
        REQUIRE(varf::GetCurrent() != varf::GetRoot());
    }

    SECTION("Push no create")
    {
        REQUIRE_FALSE(varf::Push("huh", false));
        REQUIRE(varf::GetCurrent() == varf::GetRoot());
        REQUIRE(varf::Push("yes"));
        varf::Pop();
        REQUIRE(varf::Push("yes", false));
        REQUIRE(varf::GetCurrent() != varf::GetRoot());
        varf::Pop();
    }

    SECTION("Push ..")
    {
        REQUIRE(varf::Push("special"));
        REQUIRE(varf::Push("../different"));
        REQUIRE(varf::GetCurrent() == pwd / "different");
    }

    SECTION("Push .")
    {
        REQUIRE(varf::Push("."));
        REQUIRE(varf::GetCurrent() == pwd);
        REQUIRE(varf::Push("././."));
        REQUIRE(varf::GetCurrent() == pwd);
    }

    SECTION("Push windows")
    {
        REQUIRE_THROWS(varf::Push("win\\32"));
        REQUIRE_THROWS(varf::Push("asd/\\/\\\\//"));
    }

    SECTION("Push invalid path")
    {
        REQUIRE_THROWS(varf::Push("/"));
        REQUIRE_THROWS(varf::Pop());
        REQUIRE_THROWS(varf::Push("/home"));

        REQUIRE_THROWS(varf::Push("%"));
        REQUIRE_THROWS(varf::Push("?"));
        REQUIRE_THROWS(varf::Push("%"));
        REQUIRE_THROWS(varf::Push("*"));
        REQUIRE_THROWS(varf::Push(":"));
        REQUIRE_THROWS(varf::Push("|"));
        REQUIRE_THROWS(varf::Push("\""));
        REQUIRE_THROWS(varf::Push("<"));
        REQUIRE_THROWS(varf::Push(">"));
        REQUIRE_THROWS(varf::Push(","));
        REQUIRE_THROWS(varf::Push(";"));
        REQUIRE_THROWS(varf::Push("="));
        REQUIRE_THROWS(varf::Push("not_valid."));
        REQUIRE_THROWS(varf::Push("?/%/valid"));
        REQUIRE(varf::GetCurrent() == pwd);

        if (varf::Push("file"))
        {
            std::ofstream file("exists");
            REQUIRE_THROWS(varf::Push("exists"));
            varf::Pop();
        }
        else
        {
            REQUIRE(false);
        }
    }

    varf::Reset();
    std::filesystem::remove_all(varf::GetCurrent() / "varf_tests");
}
TEST_CASE("Filemanager - Pop", "[varf][filemanager]")
{
    varf::Push("varf_tests/filemanager");
    varf::SetRoot();
    auto pwd = std::filesystem::current_path();
    SECTION("Pop when empty")
    {
        REQUIRE_THROWS(varf::Pop());
        REQUIRE_THROWS(varf::Pop(varf::POP_FULL));
        REQUIRE_THROWS(varf::Pop(25));
    }

    SECTION("Pop more than pushed")
    {
        REQUIRE(varf::Push("more than"));
        REQUIRE_THROWS(varf::Pop(2));
    }

    SECTION("Pop when push no create")
    {
        REQUIRE_FALSE(varf::Push("no create", false));
        REQUIRE_THROWS(varf::Pop());
    }

    SECTION("Pop after setting root")
    {
        REQUIRE(varf::Push("new root"));
        varf::SetRoot();
        REQUIRE_THROWS(varf::Pop());
    }

    SECTION("pop full")
    {
        REQUIRE(varf::Push("really/long/path/to/push/like/really"));
        REQUIRE_NOTHROW(varf::Pop(varf::POP_FULL));
        REQUIRE(varf::GetCurrent() == varf::GetRoot());
    }

    SECTION("pop intermediate")
    {
        REQUIRE(varf::Push("really/long/path/to/push/like/really"));
        REQUIRE_NOTHROW(varf::Pop(3));
        REQUIRE(varf::GetCurrent() == pwd / "really/long/path/to");
        REQUIRE_NOTHROW(varf::Pop(3));
        REQUIRE(varf::GetCurrent() == pwd / "really");
        REQUIRE_THROWS(varf::Pop(3));
    }

    SECTION("pop size")
    {
        REQUIRE(varf::Push("really/long/path/to/push/like/really"));
        REQUIRE_NOTHROW(varf::Pop(7));
        REQUIRE(varf::GetCurrent() == pwd);
        REQUIRE(varf::GetCurrent() == varf::GetRoot());
        REQUIRE_THROWS(varf::Pop());
    }

    SECTION("push ..")
    {
        REQUIRE(varf::Push("really/long/path/to/push"));
        REQUIRE_NOTHROW(varf::Push(".."));
        REQUIRE(varf::GetCurrent() == pwd / "really/long/path/to");
        REQUIRE_NOTHROW(varf::Push("../../.."));
        REQUIRE(varf::GetCurrent() == pwd / "really");
        REQUIRE_THROWS(varf::Push("../../.."));
    }

    varf::Reset();
    std::filesystem::remove_all(varf::GetCurrent() / "varf_tests");
}

TEST_CASE("Filemanager - SetRoot", "[varf][filemanager]")
{
    varf::Push("varf_tests/filemanager");
    varf::SetRoot();
    auto pwd = std::filesystem::current_path();
    SECTION("Invalid Root")
    {
        REQUIRE_THROWS(varf::SetRoot("asd."));
        REQUIRE(varf::GetCurrent() == pwd);
    }
    varf::Reset();
    std::filesystem::remove_all(varf::GetCurrent() / "varf_tests");
}
