#include <catch2/catch_all.hpp>

#include "FileManager/FileManager.hpp"
#include "FileManager/Serializable.hpp"
#include "FileManager/compression/Options.hpp"

#include <array>
#include <random>

using namespace varf::Compression;

struct SimpleSerialization : public varf::ISerializable
{
    int data_int_1;
    int data_int_2;
    double data_double;
    size_t data_size;

    SimpleSerialization(int d_int_1, int d_int_2, double d_double, size_t d_size)
        : data_int_1(d_int_1)
        , data_int_2(d_int_2)
        , data_double(d_double)
        , data_size(d_size)
    {
    }

#define CLASS_MEMBERS      \
    X(data_int_1, Static)  \
    X(data_int_2, Static)  \
    X(data_double, Static) \
    X(data_size, Static)

    void Serialize(std::ostream& stream) override
    {
#define X(member, type) varf::Serialize##type(stream, member);
        CLASS_MEMBERS;
#undef X
    }

    void Deserialize(std::istream& stream) override
    {
#define X(member, type) varf::Deserialize##type(stream, member);
        CLASS_MEMBERS;
#undef X
    }

#undef CLASS_MEMBERS
};

struct BigSerializable : public varf::ISerializable
{
    std::string begin{"BEGIN"};
    std::vector<uint8_t> block_1;
    std::vector<uint8_t> block_2;
    std::vector<uint8_t> block_3;
    std::string end{"END"};

#define CLASS_MEMBERS                       \
    X(begin, String)                        \
    X(block_1, ContiguousRangeStoresStatic) \
    X(block_2, ContiguousRangeStoresStatic) \
    X(block_3, ContiguousRangeStoresStatic) \
    X(end, String)

    void Serialize(std::ostream& stream) override
    {
#define X(member, type) varf::Serialize##type(stream, member);
        CLASS_MEMBERS;
#undef X
    }

    void Deserialize(std::istream& stream) override
    {
#define X(member, type) varf::Deserialize##type(stream, member);
        CLASS_MEMBERS;
#undef X
    }

#undef CLASS_MEMBERS
};

struct BiggerThanBlockSize : public varf::ISerializable
{
    std::array<uint8_t, CHUNK_SIZE * 4UL> test;

#define CLASS_MEMBERS \
    X(test, ArrayStoresStatic<uint8_t>)

    void Serialize(std::ostream& stream) override
    {
#define X(member, type) varf::Serialize##type(stream, member);
        CLASS_MEMBERS;
#undef X
    }

    void Deserialize(std::istream& stream) override
    {
#define X(member, type) varf::Deserialize##type(stream, member);
        CLASS_MEMBERS;
#undef X
    }

#undef CLASS_MEMBERS
};

TEST_CASE("Serialization -- Simple", "[varf][serial]")
{
    varf::SetRootToKnownPath("PWD");
    varf::PushFolder("varf Tests");
    SimpleSerialization s1{1, 2, 3.0, 4};
    SimpleSerialization s2{22, 1'223'341, 3123131.2323, 41'234'123'131'231'231};

    SECTION("Serialize")
    {
        varf::SetSerializeFilename("uncompressed");
        varf::Serialize(&s1);
        s1.data_int_1 = 23;
        s1.data_int_2 = 23;
        s1.data_double = 23;
        s1.data_size = 23;
        varf::Deserialize(&s1);

        REQUIRE(s1.data_int_1 == 1);
        REQUIRE(s1.data_int_2 == 2);
        REQUIRE(s1.data_double == 3.0);
        REQUIRE(s1.data_size == 4);

        varf::Serialize(&s2);
        s2 = s1;
        varf::Deserialize(&s2);

        REQUIRE(s2.data_int_1 == 22);
        REQUIRE(s2.data_int_2 == 1'223'341);
        REQUIRE(s2.data_double == 3123131.2323);
        REQUIRE(s2.data_size == 41'234'123'131'231'231);
    }

    SECTION("Serialize compression")
    {
        varf::SetSerializeFilename("compressed");
        varf::SerializeCompress(&s1);
        s1.data_int_1 = 23;
        s1.data_int_2 = 23;
        s1.data_double = 23;
        s1.data_size = 23;
        varf::DeserializeDecompress(&s1);

        REQUIRE(s1.data_int_1 == 1);
        REQUIRE(s1.data_int_2 == 2);
        REQUIRE(s1.data_double == 3.0);
        REQUIRE(s1.data_size == 4);

        varf::SerializeCompress(&s2);
        s2 = s1;
        varf::DeserializeDecompress(&s2);

        REQUIRE(s2.data_int_1 == 22);
        REQUIRE(s2.data_int_2 == 1'223'341);
        REQUIRE(s2.data_double == 3123131.2323);
        REQUIRE(s2.data_size == 41'234'123'131'231'231);
    }
    varf::PopFolder();
}

TEST_CASE("Serialization -- Multiple blocks", "[varf][serial]")
{
    varf::SetRootToKnownPath("PWD");
    varf::PushFolder("varf Tests");

    std::mt19937 mt(2'051'920); // NOLINT
    std::uniform_int_distribution<uint8_t> dist;
    BigSerializable beeg;
    beeg.block_1.resize(CHUNK_SIZE);
    beeg.block_2.resize(CHUNK_SIZE);
    beeg.block_3.resize(CHUNK_SIZE);
    for (auto& elem : beeg.block_1)
    {
        elem = dist(mt);
    }
    for (auto& elem : beeg.block_2)
    {
        elem = dist(mt);
    }
    for (auto& elem : beeg.block_3)
    {
        elem = dist(mt);
    }
    BigSerializable expected = beeg;

    SECTION("Serialize")
    {
        varf::SetSerializeFilename("uncompressed_complex");

        varf::Serialize(&beeg);

        beeg.block_1.clear();
        beeg.block_2.clear();
        beeg.block_3.clear();

        REQUIRE(beeg.block_1 != expected.block_1);
        REQUIRE(beeg.block_2 != expected.block_2);
        REQUIRE(beeg.block_3 != expected.block_3);

        varf::Deserialize(&beeg);

        REQUIRE(beeg.block_1 == expected.block_1);
        REQUIRE(beeg.block_2 == expected.block_2);
        REQUIRE(beeg.block_3 == expected.block_3);
    }

    SECTION("Serialize compress multiple blocks")
    {
        varf::SetSerializeFilename("compressed_complex");

        varf::SerializeCompress(&beeg);

        beeg.block_1.clear();
        beeg.block_2.clear();
        beeg.block_3.clear();

        REQUIRE(beeg.block_1 != expected.block_1);
        REQUIRE(beeg.block_2 != expected.block_2);
        REQUIRE(beeg.block_3 != expected.block_3);

        varf::DeserializeDecompress(&beeg);

        REQUIRE(beeg.block_1 == expected.block_1);
        REQUIRE(beeg.block_2 == expected.block_2);
        REQUIRE(beeg.block_3 == expected.block_3);
    }

    beeg.block_1.clear();
    beeg.block_2.clear();
    beeg.block_3.clear();

    varf::PopFolder();
}

TEST_CASE("Serialization - Bigger than block size", "[varf][serial]")
{
    varf::SetRootToKnownPath("PWD");
    varf::PushFolder("varf Tests");
    std::mt19937 mt(2'051'920); // NOLINT
    std::uniform_int_distribution<uint8_t> dist;
    SECTION("Serialize")
    {
        BiggerThanBlockSize test;
        BiggerThanBlockSize expected;

        for (auto& elem : test.test)
        {
            elem = dist(mt);
        }
        varf::SetSerializeFilename("compressed_bigger_block");

        expected = test;
        varf::SerializeCompress(&test);

        std::fill_n(test.test.begin(), test.test.size(), 0);

        REQUIRE(expected.test != test.test);

        varf::DeserializeDecompress(&test);

        REQUIRE(expected.test == test.test);
    }
    varf::PopFolder();
}
