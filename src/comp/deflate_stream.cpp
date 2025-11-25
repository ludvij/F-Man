// #include "internal/pch.hpp"

#include "FileManager/comp/deflate_stream.hpp"

#include <zlib.h>

namespace Fman::comp {

// shoule probably move this somewhere so it's not duplicated
auto static constexpr translate_options(CompressionOptions options)
{
    struct
    {
        int level;
        int w_bits;
        int strategy;
    } t{};

    switch (options.level)
    {
    case CompressionLevel::NO_COMPRESSION:
        t.level = Z_NO_COMPRESSION;
        break;
    case CompressionLevel::DEFAULT_COMPRESSION:
        t.level = Z_DEFAULT_COMPRESSION;
        break;
    case CompressionLevel::BEST_COMPRESSION:
        t.level = Z_BEST_COMPRESSION;
    }

    switch (options.type)
    {
    case CompressionType::RAW:
        t.w_bits = -MAX_WBITS;
        break;
    case CompressionType::ZLIB:
        t.w_bits = MAX_WBITS;
        break;
    case CompressionType::GZIP:
        t.w_bits = MAX_WBITS + 16;
        break;
    case CompressionType::DETECT:
        t.w_bits = MAX_WBITS + 32;
        break;
    }

    switch (options.strategy)
    {
    case CompressionStrategy::DEFAULT:
        t.strategy = Z_DEFAULT_STRATEGY;
        break;
    case CompressionStrategy::FILTERED:
        t.strategy = Z_FILTERED;
        break;
    case CompressionStrategy::HUFFMAN_ONLY:
        t.strategy = Z_HUFFMAN_ONLY;
        break;
    case CompressionStrategy::RLE:
        t.strategy = Z_RLE;
    default:
        break;
    }

    return t;
}

struct deflate_streambuf::Impl
{
    std::array<uint8_t, CHUNK_SIZE> buffer{};

    z_stream z_strm;
};

deflate_streambuf::deflate_streambuf(std::ostream& output_stream, CompressionOptions options)
    : m_output_stream(output_stream)
    , p_impl(new Impl)
{
    set_put_area();

    if (options.type == CompressionType::DETECT)
    {
        throw std::logic_error("Detect type can only be used in decompression");
    }
    const auto [level, w_bits, strategy] = translate_options(options);

    z_stream& z_strm = p_impl->z_strm;

    z_strm.zalloc = Z_NULL;
    z_strm.zfree = Z_NULL;
    z_strm.opaque = Z_NULL;

    if (auto err = deflateInit2(&z_strm, level, Z_DEFLATED, w_bits, 8, strategy); err != Z_OK)
    {
        throw std::runtime_error(std::format("ZLIB ERROR: {}", zError(err)));
    }
}

deflate_streambuf::~deflate_streambuf()
{
    safe_sync();
    delete p_impl;
}

deflate_streambuf::IntT deflate_streambuf::overflow(IntT ch)
{
    // sends all data to the paired stream

    compress_buffer(CHUNK_SIZE);
    // reset put area ptrs
    set_put_area();

    return TraitsT::not_eof(ch);
}

std::streamsize deflate_streambuf::xsputn(const CharT* s, std::streamsize count)
{
    std::streamsize written = 0;
    while (written < count)
    {
        if (get_available_put_area() == 0)
        {
            overflow(TraitsT::eof());
        }
        const auto avail = get_available_put_area();
        const std::streamsize to_copy = avail < (count - written) ? avail : (count - written);
        TraitsT::copy(Base::pptr(), s + written, to_copy);
        written += to_copy;
        Base::pbump(static_cast<int>(to_copy));
    }
    return written;
}

int deflate_streambuf::sync()
{
    std::streamsize total = Base::pptr() - Base::pbase();
    compress_buffer(total, true);
    return 0;
}

int deflate_streambuf::safe_sync()
{
    std::streamsize total = Base::pptr() - Base::pbase();
    try
    {
        compress_buffer(total, true);
    }
    catch (const std::runtime_error& e)
    {
        std::println("{}", e.what());
    }
    return 0;
}

void deflate_streambuf::set_put_area()
{
    char* cs = reinterpret_cast<char*>(p_impl->buffer.data());
    // whacky hack in order to accomodate extra ch in overflow
    Base::setp(cs, cs + p_impl->buffer.size());
}

std::streamsize deflate_streambuf::get_available_put_area() const
{
    return Base::epptr() - Base::pptr();
}

void deflate_streambuf::compress_buffer(const size_t sz, const bool end)
{
    int err;
    std::array<uint8_t, CHUNK_SIZE> out;

    const int flush = end ? Z_FINISH : Z_NO_FLUSH;

    auto& z_strm = p_impl->z_strm;

    z_strm.avail_in = sz;
    z_strm.next_in = p_impl->buffer.data();

    // this do while was wracking my brain for a while
    // but basically we run this until our input is completely consumed
    // this means that we can't fill our output buffer
    do
    {
        z_strm.avail_out = CHUNK_SIZE;
        z_strm.next_out = out.data();

        err = deflate(&z_strm, flush);
        if (err < Z_OK)
        {
            (void)deflateEnd(&z_strm);
            throw std::runtime_error(std::format("ZLIB ERROR: {}", zError(err)));
        }
        const std::streamsize have = CHUNK_SIZE - z_strm.avail_out;
        m_output_stream.write(reinterpret_cast<char*>(&out), have);
        if (!m_output_stream)
        {
            throw std::runtime_error("error while writing to stream");
        }
    }
    while (z_strm.avail_out == 0);

    if (end)
    {
        (void)deflateEnd(&z_strm);
        if (err != Z_STREAM_END)
        {
            throw std::runtime_error("ZLIB ERROR: Stream could not finish");
        }
    }
}

deflate_ostream::deflate_ostream(std::ostream& ostream, CompressionOptions options)
    : Base(&m_buffer)
    , m_buffer(ostream, options)
{
}

} // namespace Fman::comp
