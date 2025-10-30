#include "util/compression_utils.hpp"

#include <zlib/zlib.h>

#include <bit>
#include <filesystem>

namespace Fman
{

compression_buffer::compression_buffer(std::ostream &output_stream)
	: m_output_stream(output_stream)
{
	set_put_area();


	m_z_stream.zalloc = Z_NULL;
	m_z_stream.zfree  = Z_NULL;
	m_z_stream.opaque = Z_NULL;

	if (auto err = deflateInit(&m_z_stream, Z_DEFAULT_COMPRESSION); err != Z_OK)
	{
		throw std::runtime_error(std::format("ZLIB ERROR: {}", zError(err)));
	}
}

compression_buffer::~compression_buffer()
{
	sync();
}

compression_buffer::IntT compression_buffer::overflow(IntT ch)
{
	// sends all data to the paired stream

	if (TraitsT::eq_int_type(ch, TraitsT::eof()))
	{
		return TraitsT::eof();
	}

	// we set Base::eptr to one before the end of the buffer in order
	// to accomodate ch, this way we can follow the constraints
	// defined by std::streambuf::overflow
	m_buffer[m_buffer.size() - 1] = static_cast<uint8_t>(ch);

	compress_buffer(CHUNK_SIZE);
	// reset put area ptrs
	set_put_area();


	return TraitsT::not_eof(ch);
}

int compression_buffer::sync()
{
	compress_buffer(Base::pptr() - Base::pbase(), true);
	return 0;
}


constexpr void compression_buffer::set_put_area()
{
	char* cs = std::bit_cast<char*>(m_buffer.data());
	// whacky hack in order to accomodate extra ch in overflow
	Base::setp(cs, cs + m_buffer.size() - 1);
}



constexpr void compression_buffer::compress_buffer(const size_t sz, const bool end)
{
	int err;
	uint8_t out[CHUNK_SIZE];

	const int flush = end ? Z_FINISH : Z_NO_FLUSH;

	m_z_stream.avail_in = sz;
	m_z_stream.next_in  = m_buffer.data();

	do {

		m_z_stream.avail_out = CHUNK_SIZE;
		m_z_stream.next_out = out;

		err = deflate(&m_z_stream, flush);
		if (err < Z_OK)
		{
			deflateEnd(&m_z_stream);
			throw std::runtime_error(std::format("ZLIB ERROR: {}", zError(err)));
		}
		const size_t have = CHUNK_SIZE - m_z_stream.avail_out;
		m_output_stream.write(std::bit_cast<char*>(&out), have);
	} while (m_z_stream.avail_out == 0);

	if (end)
	{
		deflateEnd(&m_z_stream);
		if (err != Z_STREAM_END)
		{
			throw std::runtime_error("ZLIB ERROR: Stream could not finish");
		}
		deflateEnd(&m_z_stream);
	}

}

CompressionOfstream::CompressionOfstream(const std::filesystem::path& path)
	: m_ostream(path)
	, m_buffer(m_ostream)
{
	Base::rdbuf(&m_buffer);
}




}
