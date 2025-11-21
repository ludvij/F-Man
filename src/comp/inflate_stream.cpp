// #include "internal/pch.hpp"

#include "FileManager/comp/inflate_stream.hpp"

#include <zlib.h>

namespace Fman::comp {

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

struct inflate_streambuf::Impl
{
    std::array<uint8_t, CHUNK_SIZE> out_buffer{};
    std::array<uint8_t, CHUNK_SIZE> in_bufer{};
    z_stream z_strm;
};

inflate_streambuf::inflate_streambuf(std::istream& input_stream, CompressionOptions options)
    : m_input_stream(input_stream)
    , p_impl(new Impl)
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

inflate_streambuf::~inflate_streambuf()
{
    if (!m_finished)
    {
        (void)inflateEnd(&p_impl->z_strm);
    }
    delete p_impl;
}

inflate_streambuf::IntT inflate_streambuf::underflow()
{
    if (!decompress_buffer())
    {
        return TraitsT::eof();
    }

    return TraitsT::not_eof(*Base::gptr());
}

std::streamsize inflate_streambuf::xsgetn(CharT* s, std::streamsize count)
{
    std::streamsize read = 0;
    while (read < count)
    {
        if (showmanyc() == 0)
        {

            if (TraitsT::eq_int_type(underflow(), TraitsT::eof()))
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

std::streamsize inflate_streambuf::showmanyc()
{
    return Base::egptr() - Base::gptr();
}
bool inflate_streambuf::decompress_buffer()
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
        (void)inflateEnd(&z_strm);
    }

    return true;
}

void inflate_streambuf::set_get_area(const size_t sz)
{
    char* cs = reinterpret_cast<char*>(p_impl->out_buffer.data());
    Base::setg(cs, cs, cs + sz);
}

inflate_istream::inflate_istream(std::istream& istream, CompressionOptions options)
    : Base(&m_buffer)
    , m_buffer(istream, options)
{
}
} // namespace Fman::comp
