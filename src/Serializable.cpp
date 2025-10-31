#include "Serializable.hpp"

void Fman::SerializeString(std::ostream &strm, const std::string &str)
{
	SerializeStatic(strm, str.size());
	SerializeData(strm, str.data(), str.size());
}

void Fman::DeserializeString(std::istream &strm, std::string &str)
{
	size_t sz;
	DeserializeStatic(strm, sz);
	str.resize(sz);
	DeserializeData(strm, str.data(), str.size());
}

