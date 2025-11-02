#include "Serializable.hpp"

void Fman::SerializeString(std::ostream &strm, const std::string &str)
{
	SerializeContiguousRange<std::string>(strm, str);
}

void Fman::DeserializeString(std::istream &strm, std::string &str)
{
	DeserializeContiguousRange<std::string>(strm, str);
}

