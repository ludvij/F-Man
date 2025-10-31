#include <catch2/catch_all.hpp>

#include "FileManager/FileManager.hpp"
#include "FileManager/Serializable.hpp"

#include <random>
#include <array>

#include "FileManager/util/compression_utils.hpp"

struct SimpleSerialization : public Fman::ISerializable
{
	int    data_int_1;
	int    data_int_2;
	double data_double;
	size_t data_size;

	SimpleSerialization(int d_int_1, int d_int_2, double d_double, size_t d_size)
		: data_int_1(d_int_1)
		, data_int_2(d_int_2)
		, data_double(d_double)
		, data_size(d_size)
		{}


	virtual void Serialize(std::ostream& stream) override
	{
		Fman::SerializeStatic(stream, data_int_1);
		Fman::SerializeStatic(stream, data_int_2);
		Fman::SerializeStatic(stream, data_double);
		Fman::SerializeStatic(stream, data_size);
	}

	virtual void Deserialize(std::istream& stream) override
	{
		Fman::DeserializeStatic(stream, data_int_1);
		Fman::DeserializeStatic(stream, data_int_2);
		Fman::DeserializeStatic(stream, data_double);
		Fman::DeserializeStatic(stream, data_size);
	}
};

struct BigSerializable : public Fman::ISerializable
{
	std::string begin{"BEGIN"};
	std::vector<uint8_t> test;
	std::string end{"END"};

	bool operator==(const BigSerializable& rhs) const 
	{
		return test == rhs.test;
	}


	virtual void Serialize(std::ostream& stream) override
	{
		Fman::SerializeString(stream, begin);
		Fman::SerializeDynamicRangeStoresStatic<uint8_t>(stream, test);
		Fman::SerializeString(stream, end);
	}

	virtual void Deserialize(std::istream& stream) override
	{
		Fman::DeserializeString(stream, begin);
		Fman::DeserializeDynamicRangeStoresStatic<uint8_t>(stream, test);
		Fman::DeserializeString(stream, end);
	}
};

struct BiggerThanBlockSize : public Fman::ISerializable
{
	std::array<uint8_t, Fman::CHUNK_SIZE * 4> test;

	bool operator==(const BiggerThanBlockSize& rhs) const 
	{
		return test == rhs.test;
	}

	virtual void Serialize(std::ostream& stream) override
	{
		Fman::SerializeArrayStoresStatic<uint8_t>(stream, test);
	}

	virtual void Deserialize(std::istream& stream) override
	{
		Fman::DeserializeArrayStoresStatic<uint8_t>(stream, test);
	}
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

	std::mt19937 mt(2051920);
	std::uniform_int_distribution<uint8_t> dist;
	BigSerializable beeg;
	beeg.test.resize(16384 * 128);
	for(auto& elem : beeg.test)
	{
		elem = dist(mt);
	}
	BigSerializable expected = beeg;

	SECTION("Serialize")
	{
		Fman::SetSerializeFilename("uncompressed_complex");


		Fman::Serialize(&beeg);
		beeg.test.clear();

		REQUIRE(beeg != expected);
		Fman::Deserialize(&beeg);

		REQUIRE(beeg == expected);
	}

	SECTION("Serialize compress multiple blocks")
	{
		Fman::SetSerializeFilename("compressed_complex");

		Fman::SerializeCompress(&beeg);
		beeg.test.clear();

		REQUIRE(beeg != expected);
		Fman::DeserializeDecompress(&beeg);

		REQUIRE(beeg == expected);
	}
	beeg.test.clear();
	Fman::PopFolder();
}

TEST_CASE("Serialization - Bigger than block size", "[Fman][serial]")
{
	Fman::SetRootToKnownPath("PWD");
	Fman::PushFolder("Fman Tests");
	std::mt19937 mt(2051920);
	std::uniform_int_distribution<uint8_t> dist;
	SECTION("Serialize")
	{
		BiggerThanBlockSize test;
		BiggerThanBlockSize expected;
		
		for(auto& elem : test.test)
		{
			elem = dist(mt);
		}
		Fman::SetSerializeFilename("compressed_bigger_block");

		expected = test;
		Fman::SerializeCompress(&test);

		std::fill_n(test.test.begin(), test.test.size(), 0);

		REQUIRE(expected != test);

		Fman::DeserializeDecompress(&test);

		REQUIRE(expected == test);

	}
	Fman::PopFolder();
}
