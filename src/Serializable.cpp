#include "Serializable.hpp"

void Fman::SerializeString(std::ostream &strm, const std::string &str)
{
	SerializeContiguousRange(strm, str);
}

void Fman::DeserializeString(std::istream &strm, std::string &str)
{
	DeserializeContiguousRange(strm, str);
}

