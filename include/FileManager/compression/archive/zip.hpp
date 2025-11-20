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

#include <cstdint>
#include <memory>
#include <ostream>
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

    /**
     * @brief Adds data to the archive as a file
     *
     * @param stream a stream to the file to be added
     * @param name the name of the file
     * @return true If the file was appended
     * @return false If the file was not appended
     */
    bool Push(std::istream& stream, const std::string_view name);
    /**
     * @brief Creates a zip file from the archive
     *
     * @param stream a stream to the file to be saved
     * @return true If the archive was created
     * @return false If the archive was not created
     */
    bool Store(std::ostream& stream);

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
std::vector<ZippedFileDefinition> GetDirectory(std::istream& stream);

/**
 * @brief Decompresses an entry in the zip file
 *
 * @param zipped_file a zipped file definition @ref CreateZipDirectory "obtained here"
 * @param stream a stream to the archive
 * @return std::vector<uint8_t> containing decompressed file
 */
std::vector<uint8_t> DecompressFile(const ZippedFileDefinition& zipped_file, std::istream& in_stream);

} // namespace Fman::Compression

#endif // !FILE_MANAGER_ZIP_HEADER
