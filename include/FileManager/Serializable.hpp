#ifndef FILE_MANAGER_SERIALIZABLE_HEADER
#define FILE_MANAGER_SERIALIZABLE_HEADER

#include "FileManager.hpp"

#include <fstream>
#include <print>
#include <ranges>
#include <span>

namespace Fman
{



template<typename T> void SerializeStatic(std::ostream& strm, const T& t);
template<typename T> void DeserializeStatic(std::istream& strm, const T& t);

template<typename T> void SerializeArrayStoresStatic(std::ostream& strm, const std::span<T> arr);
template<typename T> void DeserializeArrayStoresStatic(std::istream& strm, std::span<T> arr);


void SerializeData(std::ostream& strm, const char* data, size_t sz);
void DeserializeData(std::istream& strm, char* data, size_t sz);

class ISerializable
{
public:
	// filestream provided in case of operation requiring it
	virtual void Serialize(std::ostream& fs) = 0;
	// filestream provided in case of operation requiring it
	virtual void Deserialize(std::istream& fs) = 0;
};


// might not be const since change can be made while serialization, rare
void Serialize(ISerializable* serial);
void Deserialize(ISerializable* serial);

void SerializeCompress(ISerializable* serial);
void DeserializeDecompress(ISerializable* serial);

void SetSerializeFilename(std::string_view name);


template<typename T>
void SerializeStatic(std::ostream& strm, const T& t)
{
	SerializeData(strm, reinterpret_cast<const char*>( &t ), sizeof t);
}

template<typename T>
void DeserializeStatic(std::istream& strm, T& t)
{
	DeserializeData(strm, reinterpret_cast<char*>( &t ), sizeof t);
}



template<typename T>
void SerializeArrayStoresStatic(std::ostream& strm, const std::span<T> arr)
{
	for (const auto& elem : arr)
	{
		SerializeData(strm, std::bit_cast<char*>( &elem ), sizeof elem);
	}
}
template<typename T>
void DeserializeArrayStoresStatic(std::istream& strm, std::span<T> arr)
{
	for (auto& elem : arr)
	{
		DeserializeData(strm, std::bit_cast<char*>( &elem ), sizeof elem);
	}
}


}

#endif