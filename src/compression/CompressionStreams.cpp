// #include "internal/pch.hpp"

#include "FileManager/compression/CompressionStreams.hpp"

#include <memory>
#include <zlib.h>

namespace Fman::Compression {

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

struct CompressionStreambuf::CompImpl
{
    std::array<uint8_t, CHUNK_SIZE> buffer{};

    z_stream z_strm;
};

CompressionStreambuf::CompressionStreambuf(std::ostream& output_stream, CompressionOptions options)
    : m_output_stream(output_stream)
    , p_impl(std::make_unique<CompImpl>())
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

CompressionStreambuf::~CompressionStreambuf() noexcept // NOLINT(bugprone-exception-escape)
{
    sync();
}

CompressionStreambuf::IntT CompressionStreambuf::overflow(IntT ch)
{
    // sends all data to the paired stream

    compress_buffer(CHUNK_SIZE);
    // reset put area ptrs
    set_put_area();

    return TraitsT::not_eof(ch);
}

std::streamsize CompressionStreambuf::xsputn(const CharT* s, std::streamsize count)
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

int CompressionStreambuf::sync()
{
    std::streamsize total = Base::pptr() - Base::pbase();
    compress_buffer(total, true);
    return 0;
}

void CompressionStreambuf::set_put_area()
{
    char* cs = reinterpret_cast<char*>(p_impl->buffer.data());
    // whacky hack in order to accomodate extra ch in overflow
    Base::setp(cs, cs + p_impl->buffer.size());
}

std::streamsize CompressionStreambuf::get_available_put_area() const
{
    return Base::epptr() - Base::pptr();
}

void CompressionStreambuf::compress_buffer(const size_t sz, const bool end)
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

CompressionOstream::CompressionOstream(std::ostream& ostream, CompressionOptions options)
    : Base(&m_buffer)
    , m_buffer(ostream, options)
{
}

struct DecompressionStreambuf::DecompImpl
{
    std::array<uint8_t, CHUNK_SIZE> out_buffer{};
    std::array<uint8_t, CHUNK_SIZE> in_bufer{};
    z_stream z_strm;
};

DecompressionStreambuf::DecompressionStreambuf(std::istream& input_stream, CompressionOptions options)
    : m_input_stream(input_stream)
    , p_impl(std::make_unique<DecompImpl>())
{
    const auto [level, w_bits, strategy] = translate_options(options);

    auto& z_strm = p_impl->z_strm;
    z_strm.zalloc = Z_NULL;
    z_strm.zfree = Z_NULL;
    z_strm.opaque = Z_NULL;
    z_strm.avail_in = 0;
    z_strm.next_in = Z_NULL;

    if (auto err = inflateInit2(&z_strm, w_bits); err != Z_OK)
    {
        throw std::runtime_error(std::format("ZLIB ERROR: {}", zError(err)));
    }
}

DecompressionStreambuf::~DecompressionStreambuf()
{
    if (!m_finished)
    {
        (void)inflateEnd(&p_impl->z_strm);
    }
}

DecompressionStreambuf::IntT DecompressionStreambuf::underflow()
{
    if (!decompress_buffer())
    {
        return TraitsT::eof();
    }

    return TraitsT::not_eof(*Base::gptr());
}

std::streamsize DecompressionStreambuf::xsgetn(CharT* s, std::streamsize count)
{
    std::streamsize read = 0;
    while (read < count)
    {
        if (showmanyc() == 0)
        {

            if (TraitsT::eq_int_type(underflow(), TraitsT::eof()) && showmanyc() == 0)
            {
                return read;
            }
        }
        const auto avail = showmanyc();
        const std::streamsize to_copy = avail < (count - read) ? avail : (count - read);
        TraitsT::copy(s + read, Base::gptr(), to_copy);
        read += to_copy;
        Base::gbump(static_cast<int>(to_copy));
    }

    return read;
}

std::streamsize DecompressionStreambuf::showmanyc()
{
    return Base::egptr() - Base::gptr();
}
bool DecompressionStreambuf::decompress_buffer()
{
    if (m_finished)
    {
        return false;
    }
    int err;

    auto& z_strm = p_impl->z_strm;

    if (z_strm.avail_in == 0)
    {
        m_input_stream.read(reinterpret_cast<char*>(p_impl->in_bufer.data()), CHUNK_SIZE);

        if (m_input_stream.bad())
        {
            m_finished = true;
            throw std::runtime_error("error while reading compression stream");
        }
        z_strm.avail_in = m_input_stream.gcount();
        z_strm.next_in = p_impl->in_bufer.data();
    }

    z_strm.avail_out = CHUNK_SIZE;
    z_strm.next_out = p_impl->out_buffer.data();
    err = inflate(&z_strm, Z_NO_FLUSH);
    switch (err)
    {
    case Z_STREAM_ERROR:
        [[fallthrough]];
    case Z_NEED_DICT:
        [[fallthrough]];
    case Z_DATA_ERROR:
        [[fallthrough]];
    case Z_MEM_ERROR:
        m_finished = true;
        (void)inflateEnd(&z_strm);
        throw std::runtime_error("ZLIB error, could not continue");
    default:
        break;
    }

    const size_t have = CHUNK_SIZE - z_strm.avail_out;

    set_get_area(have);
    if (err == Z_STREAM_END)
    {
        m_finished = true;
    }

    return m_finished;
}

void DecompressionStreambuf::set_get_area(const size_t sz)
{
    char* cs = reinterpret_cast<char*>(p_impl->out_buffer.data());
    Base::setg(cs, cs, cs + sz);
}

DecompressionIstream::DecompressionIstream(std::istream& istream, CompressionOptions options)
    : Base(&m_buffer)
    , m_buffer(istream, options)
{
}
} // namespace Fman::Compression
