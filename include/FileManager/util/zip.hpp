#ifndef FILE_MANAGER_ZIP_HEADER
#define FILE_MANAGER_ZIP_HEADER

/**
 * ZIP64 not supported right now
 * Encryption not supported right now
 * File attributes not supported right now
 *
 * this only extracts the file raw data, but the metadata is largely ignored
 * should be a fun exercise to figure that out
 */

#include "FileManager/FileManager.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace Fman::Compression {

struct ZippedFileDefinition
{
    std::string file_name;
    uint32_t offset;
    uint32_t uncompressed_size;
    uint32_t compressed_size;
    uint32_t CRC_32;
};


struct Archive
{
    std::vector<std::vector<uint8_t>> file_entries;
    std::vector<uint8_t> central_directory;
};

/**
 * @brief Creates a vector containing all directory entries contained in the zip file
 *
 * @param stream a stream to the zip file
 * @return std::vector<ZippedFileDefinition> vector containing directory entries
 */
[[nodiscard]]
std::vector<ZippedFileDefinition> CreateZipDirectory(std::istream& stream);

/**
 * @brief Decompresses an entry in the zip file
 *
 * @tparam Rng a growable contiguous range
 * @param zipped_file a zipped file definition @ref CreateZipDirectory "obtained here"
 * @param stream a stream to the zip file
 * @return Rng resulting range
 */
template <GrowableContiguosRange Rng = std::vector<uint8_t>>
[[nodiscard]]
Rng DecompressZippedFile(const ZippedFileDefinition& zipped_file, std::istream& stream);



bool AddToArchive(Archive& archive, std::istream& stream, const std::string_view name);








namespace _impl_ {
void decompress_zipped_file_impl(const ZippedFileDefinition& zipped_file, std::istream& stream, uint8_t* data);
}

template <GrowableContiguosRange Rng>
Rng DecompressZippedFile(const ZippedFileDefinition& zipped_file, std::istream& stream)
{
    Rng result;
    result.resize(zipped_file.uncompressed_size);

    stream.seekg(zipped_file.offset, std::ios::beg);

    _impl_::decompress_zipped_file_impl(zipped_file, stream, reinterpret_cast<uint8_t*>(result.data()));

    return result;
}

} // namespace Fman::Compression

#endif // !FILE_MANAGER_ZIP_HEADER
