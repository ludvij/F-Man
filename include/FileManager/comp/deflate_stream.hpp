#ifndef FILE_MANAGER_DEFLATE_STREAM_HEADER
#define FILE_MANAGER_DEFLATE_STREAM_HEADER

#include <iostream>
#include <streambuf>

#include "Options.hpp"

namespace Fman::comp {

class deflate_streambuf : public std::streambuf
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
    deflate_streambuf(std::ostream& output_stream, CompressionOptions options = {});

    // noexcept since sync can throw, if sync throws just terminate
    ~deflate_streambuf() override;

    deflate_streambuf(const deflate_streambuf&) = delete;
    deflate_streambuf& operator=(const deflate_streambuf&) = delete;
    deflate_streambuf(deflate_streambuf&&) = delete;
    deflate_streambuf& operator=(deflate_streambuf&&) = delete;

protected:

    IntT overflow(IntT ch) override;
    int sync() override;

    std::streamsize xsputn(const CharT* s, std::streamsize count) override;

private:
    void compress_buffer(size_t sz, bool end = false);

    void set_put_area();

    std::streamsize get_available_put_area() const;

    int safe_sync();

private:

    std::ostream& m_output_stream;

    // have to hide the zlib include somehow
    struct Impl;
    Impl* p_impl;
};

class deflate_ostream : public std::ostream
{
public:
    using Base = std::ostream;

public:
    deflate_ostream(std::ostream& ostream, CompressionOptions options = {});

    deflate_ostream(const deflate_ostream&) = delete;
    deflate_ostream& operator=(const deflate_ostream&) = delete;
    deflate_ostream(deflate_ostream&&) = delete;
    deflate_ostream& operator=(deflate_ostream&&) = delete;

private:
    deflate_streambuf m_buffer;
};

} // namespace Fman::comp

#endif //! FILE_MANAGER_DEFLATE_STREAM_HEADER
