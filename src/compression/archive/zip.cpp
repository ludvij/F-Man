#include "FileManager/compression/archive/zip.hpp"

#include "FileManager/FileManager.hpp"
#include "FileManager/compression/CompressionStreams.hpp"
#include "zlib.h"

#include <algorithm>
#include <cstddef>
#include <format>
#include <memory>
#include <numeric>
#include <ostream>
#include <vector>

#include <ludutils/lud_mem_stream.hpp>

#define READ_BINARY_PTR(stream, ptr, sz) stream.read(reinterpret_cast<char*>(ptr), (sz))
#define READ_BINARY(stream, var) READ_BINARY_PTR((stream), &(var), sizeof(var))

#define WRITE_BINARY_PTR(stream, ptr, sz) stream.write(reinterpret_cast<const char*>(ptr), (sz))
#define WRITE_BINARY(stream, var) WRITE_BINARY_PTR((stream), &(var), sizeof(var))

namespace Fman::Compression {

namespace _signatures_NS {
enum Enum : uint32_t
{
    LOCAL_FILE_HEADER = 0x04034B50,
    END_OF_CENTRAL_DIRECTORY_RECORD = 0x06054b50,
    CENTRAL_DIRECTORY_HEADER = 0x02014b50,
    DATA_DESCRIPTOR = 0x08074b50,
};
}; // namespace _signatures_NS

namespace _compression_NS {
enum Enum : uint16_t // NOLINT
{
    DEFLATE = 8,
    NONE = 0,
};
}; // namespace _compression_NS

using Signatures = _signatures_NS::Enum;
using Compression = _compression_NS::Enum;

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
    std::vector<uint8_t> extra_field{};
};
struct DataDescriptor
{
    uint32_t optional_signature{};
    uint32_t CRC_32{};
    uint32_t compressed_size{};
    uint32_t uncompressed_size{};
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
};
struct EndOfCentralDirectoryRecord
{
    uint32_t signature{};
    uint16_t disk_number{};
    uint16_t disk_start_number{};
    uint16_t directory_record_number_disk{};
    uint16_t directory_record_number{};
    uint32_t central_directory_size{};
    uint32_t offset{};
    uint16_t comment_length{};
    std::string comment{};
};

struct Archive::Impl
{
    struct file_entry
    {
        LocalFileHeader header;
        std::vector<uint8_t> compressed_data;
    };
    std::vector<file_entry> file_entries;
    std::vector<CentralDirectoryHeader> central_directory;

    size_t file_entries_size = 0;

    constexpr static uint32_t get_archive_entry_size(const file_entry& entry)
    {
        return entry.compressed_data.size() + 30UL + entry.header.extra_field_length + entry.header.file_name_length;
    }
};

Archive::Archive()
    : p_impl(std::make_unique<Impl>())
{
}

// https://pkwaredownloads.blob.core.windows.net/pkware-general/Documentation/APPNOTE-6.3.9.TXT

static LocalFileHeader read_local_file_header(std::istream& stream)
{
    LocalFileHeader lfh;
    READ_BINARY(stream, lfh.signature);
    READ_BINARY(stream, lfh.version);
    READ_BINARY(stream, lfh.gen_purpose_flag);
    READ_BINARY(stream, lfh.compression_method);
    READ_BINARY(stream, lfh.file_last_modification_time);
    READ_BINARY(stream, lfh.file_last_modification_date);
    READ_BINARY(stream, lfh.CRC_32);
    READ_BINARY(stream, lfh.compressed_size);
    READ_BINARY(stream, lfh.uncompressed_size);
    READ_BINARY(stream, lfh.file_name_length);
    READ_BINARY(stream, lfh.extra_field_length);

    lfh.file_name.resize(lfh.file_name_length);
    READ_BINARY_PTR(stream, lfh.file_name.data(), lfh.file_name_length);

    lfh.extra_field.resize(lfh.extra_field_length);
    READ_BINARY_PTR(stream, lfh.extra_field.data(), lfh.extra_field_length);

    return lfh;
}

static void write_local_file_header(std::ostream& stream, const LocalFileHeader& lfh)
{
    WRITE_BINARY(stream, lfh.signature);
    WRITE_BINARY(stream, lfh.version);
    WRITE_BINARY(stream, lfh.gen_purpose_flag);
    WRITE_BINARY(stream, lfh.compression_method);
    WRITE_BINARY(stream, lfh.file_last_modification_time);
    WRITE_BINARY(stream, lfh.file_last_modification_date);
    WRITE_BINARY(stream, lfh.CRC_32);
    WRITE_BINARY(stream, lfh.compressed_size);
    WRITE_BINARY(stream, lfh.uncompressed_size);
    WRITE_BINARY(stream, lfh.file_name_length);
    WRITE_BINARY(stream, lfh.extra_field_length);
    if (lfh.file_name_length > 0)
    {
        WRITE_BINARY_PTR(stream, lfh.file_name.data(), lfh.file_name_length);
    }
    if (lfh.extra_field_length > 0)
    {
        WRITE_BINARY_PTR(stream, lfh.extra_field.data(), lfh.extra_field_length);
    }
}

static CentralDirectoryHeader read_central_directory_header(std::istream& stream)
{
    CentralDirectoryHeader cdh;
    READ_BINARY(stream, cdh.signature);
    READ_BINARY(stream, cdh.version_made_by);
    READ_BINARY(stream, cdh.version_to_extract);
    READ_BINARY(stream, cdh.gen_purpose_flag);
    READ_BINARY(stream, cdh.compression_method);
    READ_BINARY(stream, cdh.file_last_modification_time);
    READ_BINARY(stream, cdh.file_last_modification_date);
    READ_BINARY(stream, cdh.CRC_32);
    READ_BINARY(stream, cdh.compressed_size);
    READ_BINARY(stream, cdh.uncompressed_size);
    READ_BINARY(stream, cdh.file_name_length);
    READ_BINARY(stream, cdh.extra_field_length);
    READ_BINARY(stream, cdh.file_comment_length);
    READ_BINARY(stream, cdh.disk_start);
    READ_BINARY(stream, cdh.internal_file_attrib);
    READ_BINARY(stream, cdh.external_file_attrib);
    READ_BINARY(stream, cdh.offset);

    cdh.file_name.resize(cdh.file_name_length);
    READ_BINARY_PTR(stream, cdh.file_name.data(), cdh.file_name_length);

    cdh.extra_field.resize(cdh.extra_field_length);
    READ_BINARY_PTR(stream, cdh.extra_field.data(), cdh.extra_field_length);

    cdh.file_comment.reserve(cdh.file_name_length);
    READ_BINARY_PTR(stream, cdh.file_comment.data(), cdh.file_comment_length);

    return cdh;
}

void write_central_directory_header(std::ostream& stream, const CentralDirectoryHeader& cdh)
{
    WRITE_BINARY(stream, cdh.signature);
    WRITE_BINARY(stream, cdh.version_made_by);
    WRITE_BINARY(stream, cdh.version_to_extract);
    WRITE_BINARY(stream, cdh.gen_purpose_flag);
    WRITE_BINARY(stream, cdh.compression_method);
    WRITE_BINARY(stream, cdh.file_last_modification_time);
    WRITE_BINARY(stream, cdh.file_last_modification_date);
    WRITE_BINARY(stream, cdh.CRC_32);
    WRITE_BINARY(stream, cdh.compressed_size);
    WRITE_BINARY(stream, cdh.uncompressed_size);
    WRITE_BINARY(stream, cdh.file_name_length);
    WRITE_BINARY(stream, cdh.extra_field_length);
    WRITE_BINARY(stream, cdh.file_comment_length);
    WRITE_BINARY(stream, cdh.disk_start);
    WRITE_BINARY(stream, cdh.internal_file_attrib);
    WRITE_BINARY(stream, cdh.external_file_attrib);
    WRITE_BINARY(stream, cdh.offset);

    if (cdh.file_name_length > 0)
    {
        WRITE_BINARY_PTR(stream, cdh.file_name.data(), cdh.file_name_length);
    }
    if (cdh.extra_field_length > 0)
    {
        WRITE_BINARY_PTR(stream, cdh.extra_field.data(), cdh.extra_field_length);
    }
    if (cdh.file_comment_length > 0)
    {
        WRITE_BINARY_PTR(stream, cdh.file_comment.data(), cdh.file_comment_length);
    }
}

static EndOfCentralDirectoryRecord read_end_of_central_directory_record(std::istream& stream)
{
    EndOfCentralDirectoryRecord eocd;

    READ_BINARY(stream, eocd.signature);
    READ_BINARY(stream, eocd.disk_number);
    READ_BINARY(stream, eocd.disk_start_number);
    READ_BINARY(stream, eocd.directory_record_number);
    READ_BINARY(stream, eocd.directory_record_number_disk);
    READ_BINARY(stream, eocd.central_directory_size);
    READ_BINARY(stream, eocd.offset);
    READ_BINARY(stream, eocd.comment_length);

    eocd.comment.resize(eocd.comment_length);
    READ_BINARY_PTR(stream, eocd.comment.data(), eocd.comment_length);

    return eocd;
}

static void write_end_of_central_directory_record(std::ostream& stream, const EndOfCentralDirectoryRecord& eocd)
{
    WRITE_BINARY(stream, eocd.signature);
    WRITE_BINARY(stream, eocd.disk_number);
    WRITE_BINARY(stream, eocd.disk_start_number);
    WRITE_BINARY(stream, eocd.directory_record_number);
    WRITE_BINARY(stream, eocd.directory_record_number_disk);
    WRITE_BINARY(stream, eocd.central_directory_size);
    WRITE_BINARY(stream, eocd.offset);
    WRITE_BINARY(stream, eocd.comment_length);

    if (eocd.comment_length > 0)
    {
        WRITE_BINARY_PTR(stream, eocd.comment.data(), eocd.comment_length);
    }
}

static DataDescriptor read_data_descriptor(std::istream& stream)
{
    DataDescriptor dd;
    READ_BINARY(stream, dd.CRC_32);
    if (dd.CRC_32 == Signatures::DATA_DESCRIPTOR)
    {
        dd.optional_signature = dd.CRC_32;
        READ_BINARY(stream, dd.CRC_32);
    }
    READ_BINARY(stream, dd.compressed_size);
    READ_BINARY(stream, dd.uncompressed_size);

    return dd;
}

constexpr static size_t find_eocd_size_from_zip(std::istream& stream)
{
    // find a signature 0x06054b50
    // advance 16 Bytes to get "offset" to start of central directory (from begginig of file)
    // check if offsset is signature of central directory header 0x02014b50
    // check that the EOCD ends at EOF
    stream.seekg(0, std::ios::end);
    const size_t file_size = stream.tellg();
    // 22 is base eocd size
    constexpr size_t max_eocd_size = 0xFFFF + 22;
    const size_t search_size = std::min(max_eocd_size, file_size);

    // starts at the end - base EOCD size, iterates until min(max_uint16_t, file_size) + base EOCD size
    constexpr size_t eocd_size = 22;
    const auto loop_end = static_cast<std::streamoff>(file_size - search_size);
    const auto loop_begin = static_cast<std::streamoff>(file_size - eocd_size);

    for (std::streamoff i = loop_begin; i >= loop_end; --i)
    {
        stream.seekg(i);
        uint32_t signature;
        READ_BINARY(stream, signature);
        if (signature == Signatures::END_OF_CENTRAL_DIRECTORY_RECORD)
        {
            // 16 is eocd start offset
            stream.seekg(i + 16);
            uint32_t offset;
            READ_BINARY(stream, offset);
            stream.seekg(offset);

            uint32_t dir_sig;
            READ_BINARY(stream, dir_sig);

            if (dir_sig != Signatures::CENTRAL_DIRECTORY_HEADER)
            {
                continue;
            }
            // 20 is comment_l_offset
            stream.seekg(i + 20);
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

std::vector<ZippedFileDefinition> GetDirectory(std::istream& stream)
{
    // get buffer containing possible eocd

    const auto eocd_size = static_cast<std::streamoff>(find_eocd_size_from_zip(stream));

    // first we need to search for the EOCD
    stream.seekg(-eocd_size, std::ios::end);
    auto eocd = read_end_of_central_directory_record(stream);

    // obtain central directory records
    stream.seekg(eocd.offset);
    const int cd_amount = eocd.directory_record_number;
    std::vector<ZippedFileDefinition> compressed_files_data;
    compressed_files_data.reserve(cd_amount);

    for (int i = 0; i < cd_amount; i++)
    {
        auto header = read_central_directory_header(stream);

        compressed_files_data.emplace_back(
            header.file_name,
            header.offset,
            header.uncompressed_size,
            header.compressed_size);
    }
    return compressed_files_data;
}

bool Archive::Push(std::istream& stream, const std::string_view name)
{
    const auto uncompressed_data = Fman::Slurp<std::vector<uint8_t>>(stream);
    p_impl->file_entries.emplace({});
    p_impl->central_directory.emplace({});
    auto& [lfh, compressed_data] = p_impl->file_entries.back();
    auto& cdh = p_impl->central_directory.back();

    uint32_t crc = crc32(0, uncompressed_data.data(), uncompressed_data.size());
    // scoped so sync is called
    {
        Lud::vector_ostream<uint8_t> vec_ostream(compressed_data);
        CompressionOstream comp_ostream(vec_ostream, {.type = CompressionType::RAW});
        WRITE_BINARY_PTR(comp_ostream, uncompressed_data.data(), uncompressed_data.size());
    }
    lfh.signature = Signatures::LOCAL_FILE_HEADER;
    lfh.version = 2; // means compressed with deflate
    lfh.gen_purpose_flag = 0;
    lfh.compression_method = Compression::DEFLATE;
    lfh.file_last_modification_time = 0; // get epoch time
    lfh.file_last_modification_date = 0; // get epoch time
    lfh.CRC_32 = crc;
    lfh.compressed_size = compressed_data.size();
    lfh.uncompressed_size = uncompressed_data.size();
    lfh.file_name_length = name.size();
    lfh.extra_field_length = 0;
    lfh.file_name = name;

    cdh.signature = Signatures::CENTRAL_DIRECTORY_HEADER;
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
    cdh.offset = p_impl->file_entries_size;
    cdh.file_name = lfh.file_name;

    p_impl->file_entries_size += Impl::get_archive_entry_size(p_impl->file_entries.back());

    return true;
}

bool Archive::Store(std::ostream& stream)
{
    const auto& [entries, central_directory, offset] = *p_impl;
    const auto eocd_size = [](uint32_t tot, const CentralDirectoryHeader& cdh) {
        return 46L + cdh.file_comment_length + cdh.file_name_length + cdh.extra_field_length + tot;
    };

    EndOfCentralDirectoryRecord eocd;
    eocd.signature = Signatures::END_OF_CENTRAL_DIRECTORY_RECORD;
    eocd.disk_number = 0;
    eocd.disk_start_number = 0;
    eocd.directory_record_number = central_directory.size();
    eocd.directory_record_number_disk = 0;
    eocd.central_directory_size = std::accumulate(central_directory.cbegin(), central_directory.cend(), 0U, eocd_size);
    eocd.offset = offset;
    eocd.comment_length = 0;

    for (const auto& entry : entries)
    {
        write_local_file_header(stream, entry.header);
        WRITE_BINARY_PTR(stream, entry.compressed_data.data(), entry.compressed_data.size());
    }
    for (const auto& directory : central_directory)
    {
        write_central_directory_header(stream, directory);
    }
    write_end_of_central_directory_record(stream, eocd);

    return true;
}

std::vector<uint8_t> DecompressFile(const ZippedFileDefinition& zipped_file, std::istream& in_stream)
{
    in_stream.seekg(zipped_file.offset, std::ios::beg);

    const LocalFileHeader lfh = read_local_file_header(in_stream);

    std::vector<uint8_t> buffer(lfh.uncompressed_size);

    if (lfh.compression_method != Compression::DEFLATE)
    {
        READ_BINARY_PTR(in_stream, buffer.data(), lfh.uncompressed_size);
    }
    else
    {
        DecompressionIstream decomp_istream(in_stream, {.type = CompressionType::RAW});

        READ_BINARY_PTR(decomp_istream, buffer.data(), lfh.uncompressed_size);
    }
#ifdef FMAN_DO_CRC_32
    Lud::check::eq(
        lfh.CRC_32,
        crc32(0, buffer.data(), lfh.uncompressed_size),
        std::format("File: [{}] is corrupted and can not be recovered", lfh.file_name));
#endif
    return buffer;

} // namespace _impl_

} // namespace Fman::Compression
