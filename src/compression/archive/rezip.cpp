namespace Fman::Compression {

namespace _signatures_NS {
enum Enum : uint32_t
{
    LOCAL_FILE_HEADER = 0x040564c,
    END_OF_CENTRAL_DIRECTORY_RECORD = 0x0605564c,
    CENTRAL_DIRECTORY_HEADER = 0x0201564c,
};
}; // namespace _signatures_NS

namespace _compression_NS {
enum Enum : uint16_t // NOLINT
{
    DEFLATE = 8,
    NONE = 0,
};
}; // namespace _compression_NS

using Signatures = _signatures_NS::Enum;
using Compression = _compression_NS::Enum;

struct LocalFileHeader
{
    uint32_t signature{};
    uint16_t compression_method{};
    uint32_t CRC_32{};
};
struct CentralDirectoryHeader
{
    uint32_t signature{};
    uint32_t compressed_size{};
    uint32_t uncompressed_size{};
    uint32_t local_file_header_position{};
    uint32_t file_name_length{};

    std::string file_name{};
};
struct EndOfCentralDirectoryRecord
{
    uint32_t signature{};
    uint16_t directory_record_number{};
    uint32_t central_directory_size{};
    uint32_t central_directory_position{};
};
} // namespace Fman::Compression
