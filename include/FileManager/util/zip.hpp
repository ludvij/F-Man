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
};

class Archive
{
public:
    Archive();

    bool Append(std::istream& stream, const std::string_view name);

private:
    struct Impl;

    std::unique_ptr<Impl> p_impl;
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

// I don't want to include zip file structures here, so this is kind of a mess
namespace _impl_ {
void decompress_zipped_file(const ZippedFileDefinition& zipped_file, std::istream& stream, uint8_t* data);
}

template <GrowableContiguosRange Rng>
Rng DecompressZippedFile(const ZippedFileDefinition& zipped_file, std::istream& stream)
{
    const auto current_pos = stream.tellg();

    Rng result;
    result.resize(zipped_file.uncompressed_size);

    _impl_::decompress_zipped_file(zipped_file, stream, reinterpret_cast<uint8_t*>(result.data()));
    stream.seekg(current_pos, std::ios::beg);
    return result;
}

} // namespace Fman::Compression

#endif // !FILE_MANAGER_ZIP_HEADER
