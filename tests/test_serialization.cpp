#include <catch2/catch_all.hpp>

#include "FileManager/FileManager.hpp"
#include "FileManager/Serializable.hpp"

#include <array>
#include <random>

#include "FileManager/util/CompressionStreams.hpp"

using namespace Fman::Compression;

struct SimpleSerialization : public Fman::ISerializable
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
#define X(member, type) Fman::Serialize##type(stream, member);
        CLASS_MEMBERS;
#undef X
    }

    void Deserialize(std::istream& stream) override
    {
#define X(member, type) Fman::Deserialize##type(stream, member);
        CLASS_MEMBERS;
#undef X
    }

#undef CLASS_MEMBERS
};

struct BigSerializable : public Fman::ISerializable
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
#define X(member, type) Fman::Serialize##type(stream, member);
        CLASS_MEMBERS;
#undef X
    }

    void Deserialize(std::istream& stream) override
    {
#define X(member, type) Fman::Deserialize##type(stream, member);
        CLASS_MEMBERS;
#undef X
    }

#undef CLASS_MEMBERS
};

struct BiggerThanBlockSize : public Fman::ISerializable
{
    std::array<uint8_t, CHUNK_SIZE * 4UL> test;

#define CLASS_MEMBERS \
    X(test, ArrayStoresStatic<uint8_t>)

    void Serialize(std::ostream& stream) override
    {
#define X(member, type) Fman::Serialize##type(stream, member);
        CLASS_MEMBERS;
#undef X
    }

    void Deserialize(std::istream& stream) override
    {
#define X(member, type) Fman::Deserialize##type(stream, member);
        CLASS_MEMBERS;
#undef X
    }

#undef CLASS_MEMBERS
};

TEST_CASE("Serialization -- Simple", "[Fman][serial]")
{
    Fman::SetRootToKnownPath("PWD");
    Fman::PushFolder("Fman Tests");
    SimpleSerialization s1{1, 2, 3.0, 4};
    SimpleSerialization s2{22, 1223341, 3123131.2323, 41234123131231231};

    SECTION("Serialize")
    {
        Fman::SetSerializeFilename("uncompressed");
        Fman::Serialize(&s1);
        s1.data_int_1 = 23;
        s1.data_int_2 = 23;
        s1.data_double = 23;
        s1.data_size = 23;
        Fman::Deserialize(&s1);

        REQUIRE(s1.data_int_1 == 1);
        REQUIRE(s1.data_int_2 == 2);
        REQUIRE(s1.data_double == 3.0);
        REQUIRE(s1.data_size == 4);

        Fman::Serialize(&s2);
        s2 = s1;
        Fman::Deserialize(&s2);

        REQUIRE(s2.data_int_1 == 22);
        REQUIRE(s2.data_int_2 == 1223341);
        REQUIRE(s2.data_double == 3123131.2323);
        REQUIRE(s2.data_size == 41234123131231231);
    }

    SECTION("Serialize compression")
    {
        Fman::SetSerializeFilename("compressed");
        Fman::SerializeCompress(&s1);
        s1.data_int_1 = 23;
        s1.data_int_2 = 23;
        s1.data_double = 23;
        s1.data_size = 23;
        Fman::DeserializeDecompress(&s1);

        REQUIRE(s1.data_int_1 == 1);
        REQUIRE(s1.data_int_2 == 2);
        REQUIRE(s1.data_double == 3.0);
        REQUIRE(s1.data_size == 4);

        Fman::SerializeCompress(&s2);
        s2 = s1;
        Fman::DeserializeDecompress(&s2);

        REQUIRE(s2.data_int_1 == 22);
        REQUIRE(s2.data_int_2 == 1223341);
        REQUIRE(s2.data_double == 3123131.2323);
        REQUIRE(s2.data_size == 41234123131231231);
    }
    Fman::PopFolder();
}

TEST_CASE("Serialization -- Multiple blocks", "[Fman][serial]")
{
    Fman::SetRootToKnownPath("PWD");
    Fman::PushFolder("Fman Tests");

    std::mt19937 mt(2051920); // NOLINT
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
        Fman::SetSerializeFilename("uncompressed_complex");

        Fman::Serialize(&beeg);

        beeg.block_1.clear();
        beeg.block_2.clear();
        beeg.block_3.clear();

        REQUIRE(beeg.block_1 != expected.block_1);
        REQUIRE(beeg.block_2 != expected.block_2);
        REQUIRE(beeg.block_3 != expected.block_3);

        Fman::Deserialize(&beeg);

        REQUIRE(beeg.block_1 == expected.block_1);
        REQUIRE(beeg.block_2 == expected.block_2);
        REQUIRE(beeg.block_3 == expected.block_3);
    }

    SECTION("Serialize compress multiple blocks")
    {
        Fman::SetSerializeFilename("compressed_complex");

        Fman::SerializeCompress(&beeg);

        beeg.block_1.clear();
        beeg.block_2.clear();
        beeg.block_3.clear();

        REQUIRE(beeg.block_1 != expected.block_1);
        REQUIRE(beeg.block_2 != expected.block_2);
        REQUIRE(beeg.block_3 != expected.block_3);

        Fman::DeserializeDecompress(&beeg);

        REQUIRE(beeg.block_1 == expected.block_1);
        REQUIRE(beeg.block_2 == expected.block_2);
        REQUIRE(beeg.block_3 == expected.block_3);
    }

    beeg.block_1.clear();
    beeg.block_2.clear();
    beeg.block_3.clear();

    Fman::PopFolder();
}

TEST_CASE("Serialization - Bigger than block size", "[Fman][serial]")
{
    Fman::SetRootToKnownPath("PWD");
    Fman::PushFolder("Fman Tests");
    std::mt19937 mt(2051920); // NOLINT
    std::uniform_int_distribution<uint8_t> dist;
    SECTION("Serialize")
    {
        BiggerThanBlockSize test;
        BiggerThanBlockSize expected;

        for (auto& elem : test.test)
        {
            elem = dist(mt);
        }
        Fman::SetSerializeFilename("compressed_bigger_block");

        expected = test;
        Fman::SerializeCompress(&test);

        std::fill_n(test.test.begin(), test.test.size(), 0);

        REQUIRE(expected.test != test.test);

        Fman::DeserializeDecompress(&test);

        REQUIRE(expected.test == test.test);
    }
    Fman::PopFolder();
}
