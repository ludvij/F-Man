#ifndef FILE_MANAGER_COMPRESSION_UTIL_HEADER
#define FILE_MANAGER_COMPRESSION_UTIL_HEADER

#include <array>
#include <filesystem>
#include <iostream>
#include <streambuf>

#include <zlib/zlib.h>

namespace Fman
{
class compression_buffer : public std::streambuf
{
public:
	constexpr static size_t CHUNK_SIZE = 16384;

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

private:
	void compress_buffer(size_t sz, bool end = false);

	void set_put_area();

private:
	std::array<uint8_t, CHUNK_SIZE> m_buffer;

	std::ostream& m_output_stream;

	z_stream m_z_stream;
};

class decompression_buffer : public std::streambuf
{
public:
	constexpr static size_t CHUNK_SIZE = 16384;

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
	// ~decompression_buffer();


protected:
	virtual IntT underflow() override;

private:
	void decompress_buffer(bool end = false);

	void set_get_area(size_t sz);

private:
	std::array<uint8_t, CHUNK_SIZE> m_buffer;

	std::istream& m_input_stream;

	z_stream m_z_stream;

	bool m_finished{ false };
};



class CompressionOfstream : public std::ostream
{
public:
	using Base = std::ostream;
public:
	CompressionOfstream(const std::filesystem::path& path);
private:
	std::ofstream m_ostream;
	Fman::compression_buffer m_buffer;
};

class DecompressionIfstream : public std::istream
{
public:
	using Base = std::istream;
public:
	DecompressionIfstream(const std::filesystem::path& path);
private:
	std::ifstream m_istream;
	Fman::decompression_buffer m_buffer;
};
}


#endif//!FILE_MANAGER_COMPRESSION_UTIL_HEADER