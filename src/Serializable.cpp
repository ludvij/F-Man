// #include "internal/pch.hpp"

#include "Serializable.hpp"

void varf::SerializeString(std::ostream& strm, const std::string& str)
{
    SerializeContiguousRangeStoresStatic(strm, str);
}

void varf::DeserializeString(std::istream& strm, std::string& str)
{
    DeserializeContiguousRangeStoresStatic(strm, str);
}
