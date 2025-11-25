#ifndef VARF_COMPRESSION_OPTIONS_HEADER
#define VARF_COMPRESSION_OPTIONS_HEADER
namespace varf::comp {

constexpr static auto CHUNK_SIZE = 16384;

enum class CompressionType : uint8_t
{
    RAW,    // Window bits from [-8, -15] set to -15
    ZLIB,   // Window bits from [ 8,  15] set to  15
    GZIP,   // Window bits from [16,  31] set to  31
    DETECT, // Window bits from [32,  47] set to  47
};

enum class CompressionLevel : uint8_t
{
    NO_COMPRESSION,
    DEFAULT_COMPRESSION,
    BEST_COMPRESSION,
};

enum class CompressionStrategy : uint8_t
{
    DEFAULT,
    FILTERED,
    HUFFMAN_ONLY,
    RLE
};

struct CompressionOptions
{
    CompressionType type = CompressionType::ZLIB;
    CompressionLevel level = CompressionLevel::DEFAULT_COMPRESSION;
    CompressionStrategy strategy = CompressionStrategy::DEFAULT;
    // int memLevel; // UNUSED
};
} // namespace varf::comp
#endif //! VARF_COMPRESSION_OPTIONS_HEADER
