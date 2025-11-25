#ifndef VARF_SERIALIZABLE_HEADER
#define VARF_SERIALIZABLE_HEADER

#include <iostream>
#include <ranges>
#include <span>

namespace varf {

/**
 * @brief Interface of a serializable object
 */
class ISerializable
{
public:
    virtual ~ISerializable() = default;
    /**
     * @brief Serializes the object to a stream
     * @param strm The stream to serialize to
     */
    virtual void Serialize(std::ostream& strm) = 0;
    /**
     * @brief Deserializes the object to a stream
     * @param strm The stream to deserialize to
     */
    virtual void Deserialize(std::istream& strm) = 0;
};

/**
 * @brief Serializes any serializable to the current serialization file
 * @param serial the serializable to be serialized
 */
void Serialize(ISerializable* serial);
/**
 * @brief Deserializes any serializable from the current serialization file
 * @param serial The serializable to be deserialized
 */
void Deserialize(ISerializable* serial);

/**
 * @brief Compresses the serializable data and sends it to the current serialization file
 * @param serial The serializable to be compressed and serialized
 */
void SerializeCompress(ISerializable* serial);
/**
 * @brief Decompresses the serializable data from the current serialization file and
 *        deserializes the serializable
 * @param serial The serializable to be decompressed and deserialized
 */
void DeserializeDecompress(ISerializable* serial);

/**
 * @brief sets the current name of the serialization file
 *        it will be appended to the current pushed folder
 *
 * @param name the name of the file
 */
void SetSerializeFilename(const std::string_view name = "srl.dat");

/**
 * @brief Serializes any static type.
 *        Any more complex type should not be serialized this way since this
 *        includes struct padding
 *
 * @tparam T the type to be serialized
 * @param strm the stream to serialize to
 * @param t the object to be serialized
 */
template <typename T>
void SerializeStatic(std::ostream& strm, const T& t);
/**
 * @brief Deserializes any static type.
 *        Any more complex type should not be deserialized this way since this
 *        includes struct padding
 * @tparam T The type to be deserialized
 * @param strm The stream to deserialize to
 * @param t The object to be deserialized
 */
template <typename T>
void DeserializeStatic(std::istream& strm, T& t);

/**
 * @brief Serilizes any viewable, contigous & fixed size range
 * @tparam T the type contained in the range
 * @param strm The stream to serialize to
 * @param arr The range to be serialized
 */
template <typename T>
void SerializeArrayStoresStatic(std::ostream& strm, const std::span<T> arr);
/**
 * @brief Deserilizes any viewable, contigous & fixed size range
 * @tparam T The type contained in the range
 * @param strm The stream to deserialize to
 * @param arr The range to be deserialized
 */
template <typename T>
void DeserializeArrayStoresStatic(std::istream& strm, std::span<T> arr);

/**
 * @brief Serializes any contigous, non-fixed size range that stores static types
 * @tparam R The type of the range
 * @param strm The stream to serialize to
 * @param range The range to serialize
 */
template <std::ranges::contiguous_range R>
void SerializeContiguousRangeStoresStatic(std::ostream& strm, const R& range);
/**
 * @brief Deserializes any contigous, non-fixed size range that stores static types
 * @tparam R The type of the range
 * @param strm The stream to deserialize to
 * @param range The range to deserialize
 */
template <std::ranges::contiguous_range R>
void DeserializeContiguousRangeStoresStatic(std::istream& strm, R& range);

template <std::ranges::range R>
void SerializeDynamicRangeStoresStatic(std::ostream& strm, const R& range);
template <std::ranges::range R>
void DeserializeDynamicRangeStoresStatic(std::istream& strm, R& range);

/**
 * @brief Serializes an std::string
 * @param strm The stream to serialize to
 * @param str The std::string to be serialized
 */
void SerializeString(std::ostream& strm, const std::string& str);
/**
 * @brief Deserializes an std::string
 * @param strm The stream to deserialize to
 * @param str The std::string to be deserialized
 */
void DeserializeString(std::istream& strm, std::string& str);

/**
 * @brief Serializes any raw data
 * @param strm The stream to seriazlie to
 * @param data The raw data to serialize
 * @param sz The size of the raw data
 */
inline void SerializeData(std::ostream& strm, const char* data, size_t sz);

/**
 * @brief Deserializes any raw data
 * @param strm The stream to deseriazlie to
 * @param data The raw data to deserialize
 * @param sz The size of the raw data
 */
inline void DeserializeData(std::istream& strm, char* data, size_t sz);

template <typename T>
void SerializeStatic(std::ostream& strm, const T& t)
{
    SerializeData(strm, reinterpret_cast<const char*>(&t), sizeof t);
}

template <typename T>
void DeserializeStatic(std::istream& strm, T& t)
{
    DeserializeData(strm, reinterpret_cast<char*>(&t), sizeof t);
}

template <typename T>
void SerializeArrayStoresStatic(std::ostream& strm, const std::span<T> arr)
{
    SerializeData(strm, reinterpret_cast<const char*>(arr.data()), sizeof(T) * arr.size());
}
template <typename T>
void DeserializeArrayStoresStatic(std::istream& strm, std::span<T> arr)
{
    DeserializeData(strm, reinterpret_cast<char*>(arr.data()), sizeof(T) * arr.size());
}

template <std::ranges::contiguous_range R>
void SerializeContiguousRangeStoresStatic(std::ostream& strm, const R& range)
{
    using T = std::ranges::range_value_t<R>;

    const size_t size = std::ranges::size(range) * sizeof(T);
    SerializeStatic(strm, size);
    SerializeData(strm, reinterpret_cast<const char*>(range.data()), size);
}

template <std::ranges::contiguous_range R>
void DeserializeContiguousRangeStoresStatic(std::istream& strm, R& range)
{
    using T = std::ranges::range_value_t<R>;
    size_t size;
    DeserializeStatic(strm, size);
    size_t container_size = size / sizeof(T);
    if (std::ranges::size(range) != container_size)
    {
        range.resize(container_size);
    }
    DeserializeData(strm, reinterpret_cast<char*>(range.data()), size);
}

template <std::ranges::range R>
void SerializeDynamicRangeStoresStatic(std::ostream& strm, const R& range)
{
    using T = std::ranges::range_value_t<R>;
    const size_t sz = std::ranges::size(range);
    SerializeStatic(strm, sz);
    for (auto it = range.begin(); it != range.end(); ++it)
    {
        SerializeStatic<T>(strm, *it);
    }
}

template <std::ranges::range R>
void DeserializeDynamicRangeStoresStatic(std::istream& strm, R& range)
{
    using T = std::ranges::range_value_t<R>;
    size_t sz;
    DeserializeStatic(strm, sz);
    range.clear();
    for (size_t i = 0; i < sz; i++)
    {
        T data;
        DeserializeStatic<T>(strm, data);
        range.insert(range.end(), data);
    }
}

inline void SerializeData(std::ostream& strm, const char* data, const size_t sz)
{
    strm.write(data, static_cast<std::streamsize>(sz));
}

inline void DeserializeData(std::istream& strm, char* data, const size_t sz)
{
    strm.read(data, static_cast<std::streamsize>(sz));
}

} // namespace varf

#endif
