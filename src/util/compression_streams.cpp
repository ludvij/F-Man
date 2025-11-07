#include "internal/pch.hpp"

#include "FileManager/util/compression_streams.hpp"

#include <zlib.h>


namespace Fman::Compression
{

#ifdef ENABLES_FILE_MANAGER_COMPRESSION_LOG
	#define FMAN_LOG(...) std::println(__VA_ARGS__);
#else
	#define FMAN_LOG(...) 
#endif// ENABLES_FILE_MANAGER_COMPRESSION_LOG



auto static constexpr translate_options(CompressionOptions options)
{
	struct {
		int level;
		int w_bits;
		int strategy;
	} t;

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
		t.w_bits =-MAX_WBITS;
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


CompressionStreambuf::CompressionStreambuf(std::ostream& output_stream, CompressionOptions options)
	: m_output_stream(output_stream)
{
	set_put_area();

	if (options.type == CompressionType::DETECT)
	{
		throw std::logic_error("Detect type can only be used in decompression");
	}
	const auto [level, w_bits, strategy] = translate_options(options);

	m_z_stream.zalloc = Z_NULL;
	m_z_stream.zfree  = Z_NULL;
	m_z_stream.opaque = Z_NULL;

	if (auto err = deflateInit2(&m_z_stream, level, Z_DEFLATED, w_bits, 8, strategy); err != Z_OK)
	{
		throw std::runtime_error(std::format("ZLIB ERROR: {}", zError(err)));
	}
}

CompressionStreambuf::~CompressionStreambuf() noexcept
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

std::streamsize CompressionStreambuf::xsputn(const CharT *s, std::streamsize count)
{
	std::streamsize written = 0;
	FMAN_LOG("[WRITE]: Sent: {} available: {}", count, get_available_put_area());
	while (written < count)
	{
		if (get_available_put_area() == 0)
		{
			FMAN_LOG("[WRITE]:   Called overflow");
			overflow();
			FMAN_LOG("[WRITE]:   Buffer size: {}", get_available_put_area());
		}
		const auto avail = get_available_put_area();
		const size_t to_copy = avail < (count - written) ? avail : (count - written);
		TraitsT::copy(Base::pptr(), s + written, to_copy);
		written += to_copy;
		Base::pbump(to_copy);
		FMAN_LOG("[WRITE]:   Wrote: {} [{}/{}]", to_copy, written, count);
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
	char* cs = reinterpret_cast<char*>( m_buffer.data() );
	// whacky hack in order to accomodate extra ch in overflow
	Base::setp(cs, cs + m_buffer.size());
}

std::streamsize CompressionStreambuf::get_available_put_area() const
{
	return Base::epptr() - Base::pptr();
}

void CompressionStreambuf::compress_buffer(const size_t sz, const bool end)
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

CompressionOstream::CompressionOstream(std::ostream& ostream, CompressionOptions options)
	: Base(&m_buffer)
	, m_buffer(ostream, options)
{
}



DecompressionStreambuf::DecompressionStreambuf(std::istream& input_stream, CompressionOptions options)
	: m_input_stream(input_stream)
{
	const auto [level, w_bits, strategy] = translate_options(options);

	m_z_stream.zalloc   = Z_NULL;
	m_z_stream.zfree    = Z_NULL;
	m_z_stream.opaque   = Z_NULL;
	m_z_stream.avail_in = 0;
	m_z_stream.next_in  = Z_NULL;

	if (auto err = inflateInit2(&m_z_stream, w_bits); err != Z_OK)
	{
		throw std::runtime_error(std::format("ZLIB ERROR: {}", zError(err)));
	}
}

DecompressionStreambuf::~DecompressionStreambuf()
{
	if (!m_finished)
	{
		(void)inflateEnd(&m_z_stream);
	}
}

DecompressionStreambuf::IntT DecompressionStreambuf::underflow()
{
	decompress_buffer();

	return TraitsT::not_eof(*Base::gptr());
}

std::streamsize DecompressionStreambuf::xsgetn(CharT *s, std::streamsize count)
{
	std::streamsize read = 0;
	FMAN_LOG("[ READ]: Requested: {} available: {}", count, showmanyc());
	while(read < count)
	{
		if (showmanyc() == 0)
		{
			FMAN_LOG("[ READ]:   Called underflow");
			underflow();
			FMAN_LOG("[ READ]:   Buffer size: {}", showmanyc());
		}
		const auto avail = showmanyc();
		const size_t to_copy = avail < (count - read) ? avail : ( count - read);
		TraitsT::copy(s + read, Base::gptr(), to_copy);
		read += to_copy;
		Base::gbump(to_copy);
		FMAN_LOG("[ READ]:   Read: {} [{}/{}]", to_copy, read, count);
	}

	return read;
}

std::streamsize DecompressionStreambuf::showmanyc()
{
	return Base::egptr() - Base::gptr();
}
void DecompressionStreambuf::decompress_buffer(bool end)
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
		m_finished = true;
		(void)inflateEnd(&m_z_stream);
		throw std::runtime_error("ZLIB error, could not continue");
	}

	const size_t have = CHUNK_SIZE - m_z_stream.avail_out;

	set_get_area(have);
	if (err == Z_STREAM_END)
	{
		m_finished = true;
	}
}

void DecompressionStreambuf::set_get_area(const size_t sz)
{
	char* cs = reinterpret_cast<char*>( m_out_buffer.data() );
	Base::setg(cs, cs, cs + sz);
}


DecompressionIstream::DecompressionIstream(std::istream& istream, CompressionOptions options)
	: Base(&m_buffer)
	, m_buffer(istream, options)
{
}
}
