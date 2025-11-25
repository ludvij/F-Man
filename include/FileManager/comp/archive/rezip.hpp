#ifndef VARF_REZIP_HEADER
#define VARF_REZIP_HEADER

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

namespace varf::comp {

/**
 * @brief A Rezip archive is an slimmed down version of a zip archive
 *        It does not duplicate any data, does not store file characteristics,
 *        does not store creation and modification time
 *        does not support encryption nor does it support anything
 *        that is not comp with zlib
 *
 *        File structure:
 *            ╔═════════════════════════════════╗
 *            ║ local file header 1             ║<─┐
 *            ╟──╂──────────────────────────────╢  │
 *            ║  ┗> compressed data 1           ║  │
 *            ╟─────────────────────────────────╢  │
 *            ║ local file header 2             ║<─│┐
 *            ╟──╂──────────────────────────────╢  ││
 *            ║  ┗> compressed data 2           ║  ││
 *            ╟───────────────///───────────────╢  ││
 *            ║ local file header n             ║<-││┐
 *            ╟──╂──────────────────────────────╢  │││
 *            ║  ┗> compressed data n           ║  │││
 *            ╠═════════════════════════════════╣  │││
 *         ┌─>║ central directory header 1      ║──┘││
 *         │  ╟─────────────────────────────────╢   ││
 *         │  ║ central directory header 2      ║───┘│
 *         │  ╟───────────────///───────────────╢    │
 *         │  ║ central directory header n      ║────┘
 *         │  ╠═════════════════════════════════╣
 *         └──║ end of central directory record ║
 *            ╚═════════════════════════════════╝
 *
 *        Data structures:
 *            ╔═ local file header ════════════════════════════╗
 *            ║ size │ name                                    ║
 *            ╠══════╪═════════════════════════════════════════╣
 *            ║      │ signature                               ║
 *            ║  4B  │┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄║
 *            ║      │     always 0x0405564C                   ║
 *            ╟──────┼─────────────────────────────────────────╢
 *            ║  4B  │ crc32 of uncompressed data              ║
 *            ╟──────┼─────────────────────────────────────────╢
 *            ║  8B  │ uncompressed size                       ║
 *            ╟──────┼─────────────────────────────────────────╢
 *            ║  8B  │ compressed size                         ║
 *            ╟──────┼─────────────────────────────────────────╢
 *            ║      │ comp method                      ║
 *            ║  1B  │┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄║
 *            ║      │     8 for DEFLATE (-MAX_WBITS)          ║
 *            ║      │     0 for none                          ║
 *            ╚══════╧═════════════════════════════════════════╝
 *
 *            ╔═ central directory header ═════════════════════╗
 *            ║ size │ name                                    ║
 *            ╠══════╪═════════════════════════════════════════╣
 *            ║      │ signature                               ║
 *            ║  4B  │┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄║
 *            ║      │ always 0x0201564C                       ║
 *            ╟──────┼─────────────────────────────────────────╢
 *            ║      │ offset to paired local file header      ║
 *            ║  8B  │┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄║
 *            ║      │     absolute from begining of file      ║
 *            ╟──────┼─────────────────────────────────────────╢
 *            ║  8B  ┢ file name length                        ║
 *            ╟──────╂─────────────────────────────────────────╢
 *            ║  nB <┩ file name                               ║
 *            ╚══════╧═════════════════════════════════════════╝
 *
 *            ╔═ end of central directory record ══════════════╗
 *            ║ size │ name                                    ║
 *            ╠══════╪═════════════════════════════════════════╣
 *            ║      │ signature                               ║
 *            ║  4B  │┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄║
 *            ║      │     always 0x0605564C                   ║
 *            ╟──────┼─────────────────────────────────────────╢
 *            ║  8B  │ size of cetral directory in bytes       ║
 *            ╟──────┼─────────────────────────────────────────╢
 *            ║      │ offset to begining of central directory ║
 *            ║  8B  │┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄║
 *            ║      │     absolute from begining of file      ║
 *            ╚══════╧═════════════════════════════════════════╝
 *
 */
class RezipArchive : public Archive
{
public:
    RezipArchive();
    /**
     * @brief Constructs an archive from a stream
     *
     * @param stream stream to archive data
     */
    RezipArchive(std::istream& stream);
    ~RezipArchive() override;

    /**
     * @brief Creates a Rezip file from the archive
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

} // namespace varf::comp

#endif // !VARF_Rezip_HEADER
