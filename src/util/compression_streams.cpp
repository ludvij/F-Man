#include "util/compression_streams.hpp"

#include <zlib.h>

#include <bit>
#include <filesystem>

namespace Fman
{

compression_buffer::compression_buffer(std::ostream& output_stream)
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

	compress_buffer(CHUNK_SIZE);
	// reset put area ptrs
	set_put_area();


	return TraitsT::not_eof(ch);
}

std::streamsize Fman::compression_buffer::xsputn(const CharT *s, std::streamsize count)
{
	std::streamsize written = 0;
	//std::println("[WRITE]: Sent: {} available: {}", count, get_available_put_area());
	while (written < count)
	{
		if (get_available_put_area() == 0)
		{
			//std::println("[WRITE]:   Called overflow");
			overflow();
			//std::println("[WRITE]:   Buffer size: {}", get_available_put_area());
		}
		const auto avail = get_available_put_area();
		const size_t to_copy = avail < (count - written) ? avail : (count - written);
		TraitsT::copy(Base::pptr(), s + written, to_copy);
		written += to_copy;
		Base::pbump(to_copy);
		//std::println("[WRITE]:   Wrote: {} [{}/{}]", to_copy, written, count);
	}
	return written;
}

int compression_buffer::sync()
{
	std::streamsize total = Base::pptr() - Base::pbase();
	compress_buffer(total, true);
	return 0;
}


void compression_buffer::set_put_area()
{
	char* cs = reinterpret_cast<char*>( m_buffer.data() );
	// whacky hack in order to accomodate extra ch in overflow
	Base::setp(cs, cs + m_buffer.size());
}

std::streamsize compression_buffer::get_available_put_area() const
{
	return Base::epptr() - Base::pptr();
}

void compression_buffer::compress_buffer(const size_t sz, const bool end)
{
	int err;
	uint8_t out[CHUNK_SIZE];

	const int flush = end ? Z_FINISH : Z_NO_FLUSH;

	m_z_stream.avail_in = sz;
	m_z_stream.next_in  = m_buffer.data();

	// this do while was wracking my brain for a while
	// but basically we run this until our input is completely consumed
	// this means that we can't fill our output buffer
	do
	{
		m_z_stream.avail_out = CHUNK_SIZE;
		m_z_stream.next_out = out;

		err = deflate(&m_z_stream, flush);
		if (err < Z_OK)
		{
			(void)deflateEnd(&m_z_stream);
			throw std::runtime_error(std::format("ZLIB ERROR: {}", zError(err)));
		}
		const size_t have = CHUNK_SIZE - m_z_stream.avail_out;
		m_output_stream.write(reinterpret_cast<char*>(&out), have);
		if (!m_output_stream)
		{
			throw std::runtime_error("error while writing to stream");
		}
	} while (m_z_stream.avail_out == 0);

	if (end)
	{
		(void)deflateEnd(&m_z_stream);
		if (err != Z_STREAM_END)
		{
			throw std::runtime_error("ZLIB ERROR: Stream could not finish");
		}
	}

}

CompressionOstream::CompressionOstream(std::ostream& ostream)
	: Base(&m_buffer)
	, m_buffer(ostream)
{
}



decompression_buffer::decompression_buffer(std::istream& input_stream)
	: m_input_stream(input_stream)
{
	m_z_stream.zalloc = Z_NULL;
	m_z_stream.zfree  = Z_NULL;
	m_z_stream.opaque = Z_NULL;

	m_z_stream.avail_in = 0;
	m_z_stream.next_out = Z_NULL;

	if (auto err = inflateInit(&m_z_stream); err != Z_OK)
	{
		throw std::runtime_error(std::format("ZLIB ERROR: {}", zError(err)));
	}
}

decompression_buffer::~decompression_buffer()
{
	if (m_finished)
	{
		(void)inflateEnd(&m_z_stream);
	}
}

decompression_buffer::IntT decompression_buffer::underflow()
{
	decompress_buffer();

	return TraitsT::not_eof(*Base::gptr());
}

std::streamsize decompression_buffer::xsgetn(CharT *s, std::streamsize count)
{
	std::streamsize read = 0;
	//std::println("[ READ]: Requested: {} available: {}", count, showmanyc());
	while(read < count)
	{
		if (showmanyc() == 0)
		{
			//std::println("[ READ]:   Called underflow");
			underflow();
			//std::println("[ READ]:   Buffer size: {}", showmanyc());
		}
		const auto avail = showmanyc();
		const size_t to_copy = avail < (count - read) ? avail : ( count - read);
		TraitsT::copy(s + read, Base::gptr(), to_copy);
		read += to_copy;
		Base::gbump(to_copy);
		//std::println("[ READ]:   Read: {} [{}/{}]", to_copy, read, count);
	}

	return read;
}

std::streamsize decompression_buffer::showmanyc()
{
	return Base::egptr() - Base::gptr();
}
void decompression_buffer::decompress_buffer(bool end)
{
	if (m_finished)
	{
		throw std::runtime_error("zlib stream finished but read was called again");
	}
	int err;


	if (m_z_stream.avail_in == 0)
	{
		m_input_stream.read(reinterpret_cast<char*>( m_in_bufer.data() ), CHUNK_SIZE);
		
		if (m_input_stream.bad())
		{
			m_finished = true;
			throw std::runtime_error("error while reading compression stream");
		}
		m_z_stream.avail_in = m_input_stream.gcount();
		m_z_stream.next_in = m_in_bufer.data();
	}


	m_z_stream.avail_out = CHUNK_SIZE;
	m_z_stream.next_out = m_out_buffer.data();
	err = inflate(&m_z_stream, Z_NO_FLUSH);
	switch (err)
	{
	case Z_STREAM_ERROR:
		[[fallthrough]];
	case Z_NEED_DICT:
		[[fallthrough]];
	case Z_DATA_ERROR:
		[[fallthrough]];
	case Z_MEM_ERROR:
		throw std::runtime_error(std::format("ZLIB ERROR: {}", zError(err)));
	}

	const size_t have = CHUNK_SIZE - m_z_stream.avail_out;

	set_get_area(have);
	if (err == Z_STREAM_END)
	{
		m_finished = true;
	}
}

void decompression_buffer::set_get_area(const size_t sz)
{
	char* cs = reinterpret_cast<char*>( m_out_buffer.data() );
	Base::setg(cs, cs, cs + sz);
}


DecompressionIstream::DecompressionIstream(std::istream& istream)
	: Base(&m_buffer)
	, m_buffer(istream)
{
}
}
