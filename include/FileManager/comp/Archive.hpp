#ifndef FILE_MANAGER_ARCHIVE_HEADER
#define FILE_MANAGER_ARCHIVE_HEADER

/**
 * ZIP64 not supported right now
 * Encryption not supported right now
 * File attributes not supported right now
 *
 * this only extracts the file raw data, but the metadata is largely ignored
 * should be a fun exercise to figure that out
 */

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

namespace Fman::comp {

struct ArchiveEntry
{
    const std::string file_name;
    const size_t index;
    const uint32_t uncompressed_size;
    const uint32_t compressed_size;
};

class Archive
{
public:

    virtual ~Archive() = default;

    /**
     * @brief Creates a zip file from the archive
     *
     * @param stream a stream to the file to be saved
     */
    virtual void Write(std::ostream& stream) const = 0;

    /**
     * @brief Adds data to the archive as a file
     *
     * @param stream a stream to the file to be added
     * @param name the name of the file
     * @throws std::runtime_error if the entry was not created
     */
    virtual void Push(std::istream& stream, const std::string_view name) = 0;

    /**
     * @brief Removes data from the archive in the form of decompressed data
     *
     * @param entry the entry to be popped
     * @return std::vector<uint8_t> Containing the data
     */
    virtual std::vector<uint8_t> Pop(const ArchiveEntry& entry) = 0;

    /**
     * @brief Obtains data from the archive without removal in the form of decompressed data
     *
     * @param entry the entry to be peeked
     * @return std::vector<uint8_t> Containing the data
     */
    [[nodiscard]]
    virtual std::vector<uint8_t> Peek(const ArchiveEntry& entry) const = 0;

    /**
     * @brief Obtains a vector containing a recollection of the archive contents
     *
     * @return std::vector<ArchiveDirectory>
     */
    [[nodiscard]]
    virtual std::vector<ArchiveEntry> GetDirectory() const = 0;
};

} // namespace Fman::comp

#endif // !FILE_MANAGER_ARCHIVE_HEADER
