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
#include <istream>
#include <ostream>
#include <vector>

#include "FileManager/comp/Archive.hpp"

namespace Fman::comp {
/**
 * @brief Very WIP zip file archive, hardly works
 *
 */
class ZipArchive : public Archive
{
public:
    ZipArchive();
    /**
     * @brief Constructs an archive from a stream
     *
     * @param stream stream to archive data
     */
    ZipArchive(std::istream& stream);
    ~ZipArchive() override;

    /**
     * @brief Creates a zip file from the archive
     *
     * @param stream a stream to the file to be saved
     */
    void Write(std::ostream& stream) const override;

    /**
     * @brief Adds data to the archive as a file
     *
     * @param stream a stream to the file to be added
     * @param name the name of the file
     * @throws std::runtime_error if the entry was not created
     */
    void Push(std::istream& stream, const std::string_view name) override;

    /**
     * @brief Removes data from the archive in the form of decompressed data
     *
     * @param entry the entry to be popped
     * @return std::vector<uint8_t> Containing the data
     */
    std::vector<uint8_t> Pop(const ArchiveEntry& entry) override;

    /**
     * @brief Obtains data from the archive without removal in the form of decompressed data
     *
     * @param entry the entry to be peeked
     * @return std::vector<uint8_t> Containing the data
     */
    std::vector<uint8_t> Peek(const ArchiveEntry& entry) const override;

    /**
     * @brief Obtains a vector containing a recollection of the archive contents
     *
     * @return std::vector<ArchiveDirectory>
     */
    std::vector<ArchiveEntry> GetDirectory() const override;

private:
    void read(std::istream& stream);

private:
    struct Impl;

    Impl* p_impl;
};

} // namespace Fman::comp

#endif // !FILE_MANAGER_ZIP_HEADER
