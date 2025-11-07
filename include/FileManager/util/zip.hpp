#ifndef FILE_MANAGER_ZIP_HEADER
#define FILE_MANAGER_ZIP_HEADER

/**
 * ZIP64 not supported right now
 * CRC-32 not supported right now
 * Encryption not supported right now
 * File attributes not supported right now
 *
 * this only extracts the file raw data, but the metadata is largely ignored
 * should be a fun exercise to figure that out
 */

#include <cstdint>
#include <vector>

namespace Fman::Compression
{
struct FileInZipData
{
	std::string file_name;
	uint32_t offset;
	uint32_t uncompressed_size;
	uint32_t compressed_size;
	uint32_t CRC_32;
};

[[nodiscard]]
std::vector<FileInZipData> CreateZipDirectory(std::istream& stream);

[[nodiscard]]
std::vector<uint8_t> DecompressZippedFile(const FileInZipData& zipped_file, std::istream& stream);

}



#endif // !FILE_MANAGER_ZIP_HEADER