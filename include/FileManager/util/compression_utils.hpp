#ifndef FILE_MANAGER_COMPRESSION_UTIL_HEADER
#define FILE_MANAGER_COMPRESSION_UTIL_HEADER

#include <array>
#include <filesystem>
#include <iostream>
#include <streambuf>

#include <zlib/zlib.h>

namespace Fman
{
constexpr static size_t CHUNK_SIZE = 16384;

class compression_buffer : public std::streambuf
{
public:

	using Base    = std::streambuf;
	using IntT    = Base::int_type;
	using TraitsT = Base::traits_type;
	using CharT   = Base::char_type;
	using OffT    = Base::off_type;
	using PosT    = Base::pos_type;
public:

	/**
	 * @brief constructs compresion buffer
	 * @param output_stream stream where the compressed data will be sent to
	 */
	compression_buffer(std::ostream& output_stream);
	~compression_buffer();

protected:

	virtual IntT overflow(IntT ch = TraitsT::eof()) override;
	virtual int sync() override;

	virtual std::streamsize xsputn(const CharT* s, std::streamsize count) override;

private:
	void compress_buffer(size_t sz, bool end = false);

	void set_put_area();

	std::streamsize get_available_put_area() const;

private:
	std::array<uint8_t, CHUNK_SIZE> m_buffer;

	std::ostream& m_output_stream;

	z_stream m_z_stream;
};

class decompression_buffer : public std::streambuf
{
public:
	using Base    = std::streambuf;
	using IntT    = Base::int_type;
	using TraitsT = Base::traits_type;
	using CharT   = Base::char_type;
	using OffT    = Base::off_type;
	using PosT    = Base::pos_type;
public:

	/**
	 * @brief constructs decompression buffer
	 * @param input_stream stream where the decompressed data will be read from
	 */
	decompression_buffer(std::istream& input_stream);
	~decompression_buffer();


protected:
	virtual IntT underflow() override;

	virtual std::streamsize xsgetn(CharT* s, std::streamsize count) override;

	virtual std::streamsize showmanyc() override;

private:
	void decompress_buffer(bool end = false);

	void set_get_area(size_t sz);

private:
	std::array<uint8_t, CHUNK_SIZE> m_out_buffer;

	std::array<uint8_t, CHUNK_SIZE> m_in_bufer;

	std::istream& m_input_stream;

	z_stream m_z_stream;

	bool m_finished{ false };
};



class CompressionOstream : public std::ostream
{
public:
	using Base = std::ostream;
public:
	CompressionOstream(std::ostream& ostream);
private:
	Fman::compression_buffer m_buffer;
};

class DecompressionIstream : public std::istream
{
public:
	using Base = std::istream;
public:
	DecompressionIstream(std::istream& istream);
private:
	Fman::decompression_buffer m_buffer;
};
}


#endif//!FILE_MANAGER_COMPRESSION_UTIL_HEADER