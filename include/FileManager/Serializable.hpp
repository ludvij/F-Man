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


void SerializeString(std::ostream& strm, const std::string& str);
void DeserializeString(std::istream& strm, std::string& str);



inline void SerializeData(std::ostream& strm, const char* data, size_t sz);
inline void DeserializeData(std::istream& strm, char* data, size_t sz);

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

void SetSerializeFilename(const std::string_view name);


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
	SerializeData(strm, std::bit_cast<char*>( arr.data() ), sizeof(T) * arr.size());
}
template<typename T>
void DeserializeArrayStoresStatic(std::istream& strm, std::span<T> arr)
{
	DeserializeData(strm, std::bit_cast<char*>(arr.data()), sizeof(T) * arr.size());
}

template <typename T, std::ranges::range R>
void SerializeDynamicRangeStoresStatic(std::ostream &strm, const R &range)
{
	const size_t sz = std::ranges::size(range);
	SerializeStatic(strm, sz);
	for(auto it = range.begin(); it != range.end(); ++it)
	{
		SerializeStatic<T>(strm, *it);
	}
}

template <typename T, std::ranges::range R>
void DeserializeDynamicRangeStoresStatic(std::istream &strm, R &range)
{
	size_t sz;
	DeserializeStatic(strm, sz);
	range.clear();
	for(size_t i = 0; i < sz; i++)
	{
		T data;
		DeserializeStatic<T>(strm, data);
		range.insert(range.end(), data);
	}
}

inline void SerializeData(std::ostream &strm, const char *data, const size_t sz)
{
	strm.write(data, sz);
}

inline void DeserializeData(std::istream& strm, char* data, const size_t sz)
{
	strm.read(data, sz);
}


}

#endif