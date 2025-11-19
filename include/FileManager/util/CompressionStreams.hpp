#ifndef FILE_MANAGER_COMPRESSION_UTIL_HEADER
#define FILE_MANAGER_COMPRESSION_UTIL_HEADER

#include <cstdint>
#include <streambuf>

namespace Fman::Compression {
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

class CompressionStreambuf : public std::streambuf
{
public:
    using Base = std::streambuf;
    using IntT = Base::int_type;
    using TraitsT = Base::traits_type;
    using CharT = Base::char_type;
    using OffT = Base::off_type;
    using PosT = Base::pos_type;

public:

    /**
     * @brief constructs compresion buffer
     * @param output_stream The stream where the compressed data will be sent to
     * @param options The options to be used by zlib
     */
    CompressionStreambuf(std::ostream& output_stream, CompressionOptions options = {});

    // noexcept since sync can throw, if sync throws just terminate
    ~CompressionStreambuf() noexcept override;

    CompressionStreambuf(const CompressionStreambuf&) = delete;
    CompressionStreambuf& operator=(const CompressionStreambuf&) = delete;
    CompressionStreambuf(CompressionStreambuf&&) = delete;
    CompressionStreambuf& operator=(CompressionStreambuf&&) = delete;

protected:

    IntT overflow(IntT ch) override;
    int sync() override;

    std::streamsize xsputn(const CharT* s, std::streamsize count) override;

private:
    void compress_buffer(size_t sz, bool end = false);

    void set_put_area();

    std::streamsize get_available_put_area() const;

private:

    std::ostream& m_output_stream;

    // have to hide the zlib include somehow
    struct CompImpl;
    std::unique_ptr<CompImpl> p_impl{};
};

class DecompressionStreambuf : public std::streambuf
{
public:
    using Base = std::streambuf;
    using IntT = Base::int_type;
    using TraitsT = Base::traits_type;
    using CharT = Base::char_type;
    using OffT = Base::off_type;
    using PosT = Base::pos_type;

public:

    /**
     * @brief constructs decompression buffer
     * @param input_stream The stream where the decompressed data will be read from
     * @param options The compression options to be used by zlib
     */
    DecompressionStreambuf(std::istream& input_stream, CompressionOptions options = {});
    ~DecompressionStreambuf() override;

    DecompressionStreambuf(const DecompressionStreambuf&) = delete;
    DecompressionStreambuf& operator=(const DecompressionStreambuf&) = delete;
    DecompressionStreambuf(DecompressionStreambuf&&) = delete;
    DecompressionStreambuf& operator=(DecompressionStreambuf&&) = delete;

protected:
    IntT underflow() override;

    std::streamsize xsgetn(CharT* s, std::streamsize count) override;

    std::streamsize showmanyc() override;

private:
    bool decompress_buffer();

    void set_get_area(size_t sz);

private:

    std::istream& m_input_stream;

    struct DecompImpl;
    std::unique_ptr<DecompImpl> p_impl;

    bool m_finished{false};
};

class CompressionOstream : public std::ostream
{
public:
    using Base = std::ostream;

public:
    CompressionOstream(std::ostream& ostream, CompressionOptions options = {});

    CompressionOstream(const CompressionOstream&) = delete;
    CompressionOstream& operator=(const CompressionOstream&) = delete;
    CompressionOstream(CompressionOstream&&) = delete;
    CompressionOstream& operator=(CompressionOstream&&) = delete;

private:
    CompressionStreambuf m_buffer;
};

class DecompressionIstream : public std::istream
{
public:
    using Base = std::istream;

public:
    DecompressionIstream(std::istream& istream, CompressionOptions options = {});

    DecompressionIstream(const DecompressionIstream&) = delete;
    DecompressionIstream& operator=(const DecompressionIstream&) = delete;
    DecompressionIstream(DecompressionIstream&&) = delete;
    DecompressionIstream& operator=(DecompressionIstream&&) = delete;

private:
    DecompressionStreambuf m_buffer;
};
} // namespace Fman::Compression

#endif //! FILE_MANAGER_COMPRESSION_UTIL_HEADER
