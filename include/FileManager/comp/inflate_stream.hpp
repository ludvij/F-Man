#ifndef VARF_INFLATE_STREAM_HEADER
#define VARF_INFLATE_STREAM_HEADER
#include "Options.hpp"

namespace varf::comp {

class inflate_streambuf : public std::streambuf
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
     * @brief constructs Inflate buffer
     * @param input_stream The stream where the decompressed data will be read from
     * @param options The compression options to be used by zlib
     */
    inflate_streambuf(std::istream& input_stream, CompressionOptions options = {});
    ~inflate_streambuf() override;

    inflate_streambuf(const inflate_streambuf&) = delete;
    inflate_streambuf& operator=(const inflate_streambuf&) = delete;
    inflate_streambuf(inflate_streambuf&&) = delete;
    inflate_streambuf& operator=(inflate_streambuf&&) = delete;

protected:
    IntT underflow() override;

    std::streamsize xsgetn(CharT* s, std::streamsize count) override;

    std::streamsize showmanyc() override;

private:
    bool decompress_buffer();

    void set_get_area(size_t sz);

private:

    std::istream& m_input_stream;

    struct Impl;
    Impl* p_impl;

    bool m_finished{false};
};

class inflate_istream : public std::istream
{
public:
    using Base = std::istream;

public:
    inflate_istream(std::istream& istream, CompressionOptions options = {});

    inflate_istream(const inflate_istream&) = delete;
    inflate_istream& operator=(const inflate_istream&) = delete;
    inflate_istream(inflate_istream&&) = delete;
    inflate_istream& operator=(inflate_istream&&) = delete;

private:
    inflate_streambuf m_buffer;
};
} // namespace varf::comp
#endif //! VARF_INFLATE_STREAM_HEADER
