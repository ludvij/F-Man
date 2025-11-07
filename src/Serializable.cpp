#include "internal/pch.hpp"

#include "FileManager/Serializable.hpp"

void Fman::SerializeString(std::ostream &strm, const std::string& str)
{
	SerializeContiguousRangeStoresStatic(strm, str);
}

void Fman::DeserializeString(std::istream &strm, std::string &str)
{
	DeserializeContiguousRangeStoresStatic(strm, str);
}

