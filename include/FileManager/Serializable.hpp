#ifndef FILE_MANAGER_SERIALIZABLE_HEADER
#define FILE_MANAGER_SERIALIZABLE_HEADER

#include "FileManager.hpp"

#include <fstream>
#include <ranges>
#include <span>

namespace Fman
{



template<typename T> void SerializeStatic(const T& t);
template<typename T> void DeserializeStatic(const T& t);

template<typename T> void SerializeArrayStoresStatic(const std::span<T> arr);
template<typename T> void DeserializeArrayStoresStatic(std::span<T> arr);


void SerializeData(const char* data, size_t sz);
void DeserializeData(char* data, size_t sz);

class ISerializable
{
public:
	// filestream provided in case of operation requiring it
	virtual void Serialize(std::fstream& fs) = 0;
	// filestream provided in case of operation requiring it
	virtual void Deserialize(std::fstream& fs) = 0;
};


// might not be const since change can be made while serialization, rare
void Serialize(ISerializable* serial);
void Deserialize(ISerializable* serial);

void SetSerializeFilename(std::string_view name);


template<typename T>
void SerializeStatic(const T& t)
{
	SerializeData(reinterpret_cast<const char*>( &t ), sizeof t);
}

template<typename T>
void DeserializeStatic(T& t)
{
	DeserializeData(reinterpret_cast<char*>( &t ), sizeof t);
}



template<typename T>
void SerializeArrayStoresStatic(const std::span<T> arr)
{
	for (const auto& elem : arr)
	{
		SerializeData(std::bit_cast<char*>( &elem ), sizeof elem);
	}
}
template<typename T>
void DeserializeArrayStoresStatic(std::span<T> arr)
{
	for (auto& elem : arr)
	{
		DeserializeData(std::bit_cast<char*>( &elem ), sizeof elem);
	}
}


}

#endif