#include <catch2/catch_all.hpp>

#include "FileManager/FileManager.hpp"
#include "FileManager/Serializable.hpp"


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


TEST_CASE("Serialization -- Simple", "[Fman][serial]")
{
	Fman::SetRootToKnownPath("PWD");
	Fman::PushFolder("Fman Tests");
	SECTION("Serialize")
	{
		Fman::SetSerializeFilename("uncompressed");
		SimpleSerialization s1{1, 2, 3.0, 4};
		SimpleSerialization s2{22, 1223341, 3123131.2323, 41234123131231231};
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
		SimpleSerialization s1{1, 2, 3.0, 4};
		SimpleSerialization s2{22, 1223341, 3123131.2323, 41234123131231231};
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