// #include "internal/pch.hpp"

#include "FileManager/util/zip.hpp"

#include "FileManager/FileManager.hpp"
#include "FileManager/util/CompressionStreams.hpp"
#include "zlib.h"
#include <algorithm>
#include <cstddef>
#include <format>
#include <ludutils/lud_mem_stream.hpp>
#include <numeric>
#include <vector>

#define READ_BINARY_PTR(stream, ptr, sz) stream.read(reinterpret_cast<char*>(ptr), (sz))
#define READ_BINARY(stream, var) READ_BINARY_PTR((stream), &(var), sizeof(var))

#define WRITE_BINARY_PTR(stream, ptr, sz) stream.write(reinterpret_cast<const char*>(ptr), (sz))
#define WRITE_BINARY(stream, var) WRITE_BINARY_PTR((stream), &(var), sizeof(var))

namespace Fman::Compression {

// https://pkwaredownloads.blob.core.windows.net/pkware-general/Documentation/APPNOTE-6.3.9.TXT
struct LocalFileHeader
{
    uint32_t signature{};
    uint16_t version{};
    uint16_t gen_purpose_flag{};
    uint16_t compression_method{};
    uint16_t file_last_modification_time{};
    uint16_t file_last_modification_date{};
    uint32_t CRC_32{};
    uint32_t compressed_size{};
    uint32_t uncompressed_size{};
    uint16_t file_name_length{};
    uint16_t extra_field_length{};

    std::string file_name{};
    // made a ptr so it can be used with memory_istream
    // maybe looking for better solutions
    std::vector<uint8_t> extra_field{};

    static constexpr uint16_t COMPRESSION_DEFLATE = 8;
    static constexpr uint16_t COMPRESSION_NONE = 0;

    static constexpr uint32_t SIGNATURE = 0x04034B50;

    LocalFileHeader(const LocalFileHeader& other) = delete;
    LocalFileHeader(LocalFileHeader&& other) = delete;
    LocalFileHeader& operator=(const LocalFileHeader& other) = delete;
    LocalFileHeader& operator=(LocalFileHeader&& other) = delete;

    LocalFileHeader() = default;

    explicit LocalFileHeader(std::istream& stream)
    {
        READ_BINARY(stream, signature);
        READ_BINARY(stream, version);
        READ_BINARY(stream, gen_purpose_flag);
        READ_BINARY(stream, compression_method);
        READ_BINARY(stream, file_last_modification_time);
        READ_BINARY(stream, file_last_modification_date);
        READ_BINARY(stream, CRC_32);
        READ_BINARY(stream, compressed_size);
        READ_BINARY(stream, uncompressed_size);
        READ_BINARY(stream, file_name_length);
        READ_BINARY(stream, extra_field_length);

        file_name.resize(file_name_length);
        READ_BINARY_PTR(stream, file_name.data(), file_name_length);

        extra_field.resize(extra_field_length);
        READ_BINARY_PTR(stream, extra_field.data(), extra_field_length);
    }
};

struct CentralDirectoryHeader
{
    uint32_t signature{};
    uint16_t version_made_by{};
    uint16_t version_to_extract{};
    uint16_t gen_purpose_flag{};
    uint16_t compression_method{};
    uint16_t file_last_modification_time{};
    uint16_t file_last_modification_date{};
    uint32_t CRC_32{};
    uint32_t compressed_size{};
    uint32_t uncompressed_size{};
    uint16_t file_name_length{};
    uint16_t extra_field_length{};
    uint16_t file_comment_length{};
    uint16_t disk_start{};
    uint16_t internal_file_attrib{};
    uint32_t external_file_attrib{};
    uint32_t offset{};

    std::string file_name{};
    std::vector<uint8_t> extra_field{};
    std::vector<uint8_t> file_comment{};

    static constexpr uint32_t SIGNATURE = 0x02014b50;

    CentralDirectoryHeader() = default;

    explicit CentralDirectoryHeader(std::istream& stream)
    {
        READ_BINARY(stream, signature);
        READ_BINARY(stream, version_made_by);
        READ_BINARY(stream, version_to_extract);
        READ_BINARY(stream, gen_purpose_flag);
        READ_BINARY(stream, compression_method);
        READ_BINARY(stream, file_last_modification_time);
        READ_BINARY(stream, file_last_modification_date);
        READ_BINARY(stream, CRC_32);
        READ_BINARY(stream, compressed_size);
        READ_BINARY(stream, uncompressed_size);
        READ_BINARY(stream, file_name_length);
        READ_BINARY(stream, extra_field_length);
        READ_BINARY(stream, file_comment_length);
        READ_BINARY(stream, disk_start);
        READ_BINARY(stream, internal_file_attrib);
        READ_BINARY(stream, external_file_attrib);
        READ_BINARY(stream, offset);

        extra_field.reserve(extra_field_length);

        file_name.resize(file_name_length);
        READ_BINARY_PTR(stream, file_name.data(), file_name_length);

        READ_BINARY_PTR(stream, extra_field.data(), extra_field_length);

        file_comment.reserve(file_name_length);
        READ_BINARY_PTR(stream, file_comment.data(), file_comment_length);
    }
};

struct EndOfCentralDirectoryRecord
{
    uint32_t signature{};
    uint16_t disk_number{};
    uint16_t disk_start_number{};
    uint16_t directory_record_number{};
    uint16_t directory_record_number_disk{};
    uint32_t central_directory_size{};
    uint32_t offset{};
    uint16_t comment_length{};
    std::string comment{};

    static constexpr uint32_t SIGNATURE = 0x06054b50;
    static constexpr std::streamoff OFFSET_OFFSET = 16;
    static constexpr std::streamoff COMMENT_L_OFFSET = 20;
    static constexpr size_t BASE_SIZE = 22;

    explicit EndOfCentralDirectoryRecord(std::istream& stream)
    {
        READ_BINARY(stream, signature);
        READ_BINARY(stream, disk_number);
        READ_BINARY(stream, disk_start_number);
        READ_BINARY(stream, directory_record_number);
        READ_BINARY(stream, directory_record_number_disk);
        READ_BINARY(stream, central_directory_size);
        READ_BINARY(stream, offset);
        READ_BINARY(stream, comment_length);

        char* buf = new char[comment_length];

        READ_BINARY_PTR(stream, buf, comment_length);
        comment = std::string(buf, comment_length);

        delete[] buf;
    }
};

struct DataDescriptor
{
    uint32_t optional_signature{};
    uint32_t CRC_32{};
    uint32_t compressed_size{};
    uint32_t uncompressed_size{};

    static constexpr uint32_t SIGNATURE = 0x08074b50;

    DataDescriptor() = default;

    explicit DataDescriptor(std::istream& stream)
    {
        READ_BINARY(stream, CRC_32);
        if (CRC_32 == SIGNATURE)
        {
            optional_signature = CRC_32;
            READ_BINARY(stream, CRC_32);
        }
        READ_BINARY(stream, compressed_size);
        READ_BINARY(stream, uncompressed_size);
    }
};

constexpr static size_t find_eocd_size(std::istream& stream)
{
    // find a signature 0x06054b50
    // advance 16 Bytes to get "offset" to start of central directory (from begginig of file)
    // check if offsset is signature of central directory header 0x02014b50
    // check that the EOCD ends at EOF
    stream.seekg(0, std::ios::end);
    const size_t file_size = stream.tellg();
    constexpr size_t max_eocd_size = 0xFFFF + EndOfCentralDirectoryRecord::BASE_SIZE;
    const size_t search_size = std::min(max_eocd_size, file_size);

    // starts at the end - base EOCD size, iterates until min(max_uint16_t, file_size) + base EOCD size
    constexpr size_t eocd_size = EndOfCentralDirectoryRecord::BASE_SIZE;
    const auto loop_end = static_cast<std::streamoff>(file_size - search_size);
    const auto loop_begin = static_cast<std::streamoff>(file_size - eocd_size);

    for (std::streamoff i = loop_begin; i >= loop_end; --i)
    {
        stream.seekg(i);
        int signature;
        READ_BINARY(stream, signature);
        if (signature == EndOfCentralDirectoryRecord::SIGNATURE)
        {
            stream.seekg(i + EndOfCentralDirectoryRecord::OFFSET_OFFSET);
            uint32_t offset;
            READ_BINARY(stream, offset);
            stream.seekg(offset);

            uint32_t dir_sig;
            READ_BINARY(stream, dir_sig);

            if (dir_sig != CentralDirectoryHeader::SIGNATURE)
            {
                continue;
            }
            stream.seekg(i + EndOfCentralDirectoryRecord::COMMENT_L_OFFSET);
            uint16_t comment_sz;
            READ_BINARY(stream, comment_sz);
            // eocd not at the end of file
            if (i + eocd_size + comment_sz != file_size)
            {
                continue;
            }
            return eocd_size + comment_sz;
        }
    }
    // should never happen
    throw std::runtime_error("unable to find EOCD");
}

std::vector<ZippedFileDefinition> CreateZipDirectory(std::istream& stream)
{
    // get buffer containing possible eocd

    const auto eocd_size = static_cast<std::streamoff>(find_eocd_size(stream));

    // first we need to search for the EOCD
    stream.seekg(-eocd_size, std::ios::end);
    auto eocd = EndOfCentralDirectoryRecord(stream);

    // obtain central directory records
    stream.seekg(eocd.offset);
    const int cd_amount = eocd.directory_record_number;
    std::vector<ZippedFileDefinition> compressed_files_data;
    compressed_files_data.reserve(cd_amount);

    for (int i = 0; i < cd_amount; i++)
    {
        auto header = CentralDirectoryHeader(stream);

        compressed_files_data.emplace_back(
            header.file_name,
            header.offset,
            header.uncompressed_size,
            header.compressed_size,
            header.CRC_32);
    }
    return compressed_files_data;
}



bool AddToArchive(Archive& archive, std::istream& stream, const std::string_view name)
{
    LocalFileHeader lfh;
    lfh.signature = LocalFileHeader::SIGNATURE;
    lfh.version = 2.0; // means compressed with deflate
    lfh.gen_purpose_flag = 0;
    lfh.compression_method = LocalFileHeader::COMPRESSION_DEFLATE;
    lfh.file_last_modification_date = 0; // get epoch time
    lfh.file_last_modification_time = 0; // get epoch time

    const auto uncompressed_data = Fman::Slurp<std::vector<uint8_t>>(stream);

    lfh.CRC_32 = crc32(0, uncompressed_data.data(), uncompressed_data.size());

    std::vector<uint8_t> compressed_data;
    Lud::vector_ostream<uint8_t> vec_ostream(compressed_data);
    {
        // scoped so sync is called
        CompressionOstream comp_ostream(vec_ostream, {.type = CompressionType::RAW});
        WRITE_BINARY_PTR(comp_ostream, uncompressed_data.data(), uncompressed_data.size());
    }
    lfh.compressed_size = compressed_data.size();
    lfh.uncompressed_size = uncompressed_data.size();
    lfh.file_name_length = name.size();
    lfh.extra_field_length = 0;
    lfh.file_name = name;

    CentralDirectoryHeader cdh;
    cdh.signature = CentralDirectoryHeader::SIGNATURE;
    cdh.version_made_by = 0; //????
    cdh.version_to_extract = 2.0;
    cdh.compressed_size = compressed_data.size();
    cdh.file_last_modification_date = lfh.file_last_modification_date;
    cdh.file_last_modification_time = lfh.file_last_modification_time;
    cdh.CRC_32 = lfh.CRC_32;
    cdh.compressed_size = lfh.compressed_size;
    cdh.uncompressed_size = lfh.uncompressed_size;
    cdh.file_name_length = lfh.file_name_length;
    cdh.extra_field_length = 0;
    cdh.file_comment_length = 0;
    cdh.disk_start = 0;
    cdh.internal_file_attrib = 0;
    cdh.external_file_attrib = 0;

    cdh.offset = std::transform_reduce(
        archive.file_entries.cbegin(),
        archive.file_entries.cend(),
        0,
        [](size_t sz, size_t tot) { return tot + sz; },
        [](const auto& vec) { return vec.size(); });

    cdh.file_name = lfh.file_name;

    return true;
}


namespace _impl_ {
void decompress_zipped_file_impl(const ZippedFileDefinition& zipped_file, std::istream& stream, uint8_t* data)
{
    stream.seekg(zipped_file.offset, std::ios::beg);

    const LocalFileHeader lfh(stream);

    if (lfh.compression_method != LocalFileHeader::COMPRESSION_DEFLATE)
    {
        READ_BINARY_PTR(stream, data, lfh.uncompressed_size);
    }
    else
    {
        DecompressionIstream decomp_istream(stream, {.type = CompressionType::RAW});

        READ_BINARY_PTR(decomp_istream, data, lfh.uncompressed_size);
    }
#ifdef FMAN_DO_CRC_32
    Lud::check::eq(
        zipped_file.CRC_32,
        crc32(0, data, zipped_file.uncompressed_size),
        std::format("File: [{}] is corrupted and can not be recovered", zipped_file.file_name));
#endif
}

} // namespace _impl_

} // namespace Fman::Compression
