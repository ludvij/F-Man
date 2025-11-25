#include "FileManager/comp/archive/zip.hpp"
#include "FileManager/comp/deflate_stream.hpp"
#include "FileManager/comp/inflate_stream.hpp"

#include <ludutils/lud_assert.hpp>
#include <ludutils/lud_mem_stream.hpp>

#include <zlib.h>

#define READ_BINARY_PTR(stream, ptr, sz) stream.read(reinterpret_cast<char*>(ptr), (sz))
#define READ_BINARY(stream, var) READ_BINARY_PTR((stream), &(var), sizeof(var))

#define WRITE_BINARY_PTR(stream, ptr, sz) stream.write(reinterpret_cast<const char*>(ptr), (sz))
#define WRITE_BINARY(stream, var) WRITE_BINARY_PTR((stream), &(var), sizeof(var))

namespace varf::comp {

namespace {
namespace signatures_NS {
enum Enum : uint32_t
{
    LOCAL_FILE_HEADER = 0x04034B50,
    END_OF_CENTRAL_DIRECTORY_RECORD = 0x06054b50,
    CENTRAL_DIRECTORY_HEADER = 0x02014b50,
    DATA_DESCRIPTOR = 0x08074b50,
};
}; // namespace signatures_NS

namespace compression_method_NS {
enum Enum : uint16_t // NOLINT
{
    DEFLATE = 8,
    NONE = 0,
};
}; // namespace compression_method_NS

using Signatures = signatures_NS::Enum;
using CompressionMethod = compression_method_NS::Enum;

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
} // namespace
static std::vector<uint8_t> slurp(std::istream& stream)
{
    const auto current_pos = stream.tellg();
    stream.seekg(0, std::ios::end);
    std::vector<uint8_t> data(stream.tellg());
    stream.seekg(current_pos, std::ios::beg);
    READ_BINARY_PTR(stream, data.data(), data.size());

    return data;
}

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

    if (lfh.file_name_length > 0)
    {
        lfh.file_name.resize(lfh.file_name_length);
        READ_BINARY_PTR(stream, lfh.file_name.data(), lfh.file_name_length);
    }

    if (lfh.extra_field_length > 0)
    {
        lfh.extra_field.resize(lfh.extra_field_length);
        READ_BINARY_PTR(stream, lfh.extra_field.data(), lfh.extra_field_length);
    }

    Lud::check::that(
        lfh.signature == Signatures::LOCAL_FILE_HEADER,
        "Incorrect local file header signature"
    );

    Lud::check::in(
        lfh.compression_method,
        {CompressionMethod::DEFLATE, CompressionMethod::NONE},
        "Unknown compression method"
    );

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

static constexpr uint32_t get_local_file_header_size(const LocalFileHeader& lfh)
{
    return 30U + lfh.file_name_length + lfh.extra_field_length;
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

    if (cdh.file_name_length > 0)
    {
        cdh.file_name.resize(cdh.file_name_length);
        READ_BINARY_PTR(stream, cdh.file_name.data(), cdh.file_name_length);
    }

    if (cdh.extra_field_length > 0)
    {
        cdh.extra_field.resize(cdh.extra_field_length);
        READ_BINARY_PTR(stream, cdh.extra_field.data(), cdh.extra_field_length);
    }

    if (cdh.file_comment_length > 0)
    {
        cdh.file_comment.reserve(cdh.file_comment_length);
        READ_BINARY_PTR(stream, cdh.file_comment.data(), cdh.file_comment_length);
    }

    const auto current_pos = stream.tellg();
    stream.seekg(static_cast<std::streamoff>(cdh.offset), std::ios::beg);
    uint32_t lfh_signature;
    READ_BINARY(stream, lfh_signature);
    stream.seekg(current_pos, std::ios::beg);

    Lud::check::that(
        cdh.signature == Signatures::CENTRAL_DIRECTORY_HEADER,
        "Incorrect central directory header signature"
    );

    Lud::check::that(
        lfh_signature == Signatures::LOCAL_FILE_HEADER,
        "central directory header offset does not point to local file header"
    );

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

static constexpr uint32_t get_central_directory_header_size(const CentralDirectoryHeader& cdh)
{
    return 46U + cdh.file_name_length + cdh.extra_field_length + cdh.file_comment_length;
}

static EndOfCentralDirectoryRecord read_end_of_central_directory_record(std::istream& stream)
{
    const size_t begin_size = stream.tellg();
    EndOfCentralDirectoryRecord eocd;

    READ_BINARY(stream, eocd.signature);
    READ_BINARY(stream, eocd.disk_number);
    READ_BINARY(stream, eocd.disk_start_number);
    READ_BINARY(stream, eocd.directory_record_number);
    READ_BINARY(stream, eocd.directory_record_number_disk);
    READ_BINARY(stream, eocd.central_directory_size);
    READ_BINARY(stream, eocd.offset);
    READ_BINARY(stream, eocd.comment_length);

    if (eocd.comment_length > 0)
    {
        eocd.comment.resize(eocd.comment_length);
        READ_BINARY_PTR(stream, eocd.comment.data(), eocd.comment_length);
    }

    const auto current_pos = stream.tellg();
    stream.seekg(static_cast<std::streamoff>(eocd.offset), std::ios::beg);
    uint32_t cdh_signature;
    READ_BINARY(stream, cdh_signature);
    stream.seekg(current_pos, std::ios::beg);

    Lud::check::that(
        eocd.signature == Signatures::END_OF_CENTRAL_DIRECTORY_RECORD,
        "Incorrect EOCD signature"
    );

    Lud::check::that(
        cdh_signature == Signatures::CENTRAL_DIRECTORY_HEADER,
        "EOCD offset does not point to central directory header"
    );

    Lud::check::that(
        begin_size - eocd.offset == eocd.central_directory_size,
        "Incorrect EOCD directory size"
    );

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

static size_t find_eocd_size_from_zip(std::istream& stream)
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

struct ZipArchive::Impl
{
    struct file_entry
    {
        LocalFileHeader header;
        std::vector<uint8_t> compressed_data;
    };
    std::vector<file_entry> file_entries;
};

ZipArchive::ZipArchive()
    : p_impl(new Impl)
{
}

ZipArchive::ZipArchive(std::istream& stream)
    : ZipArchive()
{
    read(stream);
}

ZipArchive::~ZipArchive()
{
    delete p_impl;
}

// https://pkwaredownloads.blob.core.windows.net/pkware-general/Documentation/APPNOTE-6.3.9.TXT

void ZipArchive::Push(std::istream& stream, const std::string_view name)
{
    auto uncompressed_data = slurp(stream);

    p_impl->file_entries.emplace_back();

    auto& [lfh, compressed_data] = p_impl->file_entries.back();

    uint32_t crc = crc32(0, uncompressed_data.data(), uncompressed_data.size());
    if (!uncompressed_data.empty())
    {
        // zlib recommends to set the buffer size to at least the uncompressed size
        compressed_data.reserve(uncompressed_data.size());

        // scoped so sync is automatically called on destruction
        {
            Lud::vector_ostream vec_ostream(compressed_data);
            deflate_ostream comp_ostream(vec_ostream, {.type = CompressionType::RAW});

            WRITE_BINARY_PTR(comp_ostream, uncompressed_data.data(), uncompressed_data.size());
        }
        if (compressed_data.size() >= uncompressed_data.size())
        {
            compressed_data = std::move(uncompressed_data);
            lfh.compression_method = CompressionMethod::NONE;
            lfh.uncompressed_size = compressed_data.size();
            lfh.compressed_size = compressed_data.size();
        }
        else
        {
            lfh.compression_method = CompressionMethod::DEFLATE;
            lfh.compressed_size = compressed_data.size();
            lfh.uncompressed_size = uncompressed_data.size();
        }
    }
    else
    {
        lfh.compression_method = CompressionMethod::NONE;
        lfh.uncompressed_size = compressed_data.size();
        lfh.compressed_size = compressed_data.size();
    }
    lfh.signature = Signatures::LOCAL_FILE_HEADER;
    lfh.version = 2; // means compressed with deflate
    lfh.gen_purpose_flag = 0;
    lfh.file_last_modification_time = 0; // get epoch time
    lfh.file_last_modification_date = 0; // get epoch time
    lfh.CRC_32 = crc;
    lfh.file_name_length = name.size();
    lfh.extra_field_length = 0;
    lfh.file_name = name;
}

std::vector<uint8_t> ZipArchive::Peek(const ArchiveEntry& entry) const
{
    auto& entries = p_impl->file_entries;

    auto& [lfh, compressed_data] = entries[entry.index];
    std::vector<uint8_t> uncompressed_data;
    uncompressed_data.resize(lfh.uncompressed_size);
    if (lfh.compression_method == CompressionMethod::NONE)
    {
        std::copy_n(compressed_data.begin(), lfh.uncompressed_size, uncompressed_data.begin());
    }
    else
    {
        Lud::memory_istream<uint8_t> mem_stream(compressed_data);
        inflate_istream inflate_stream(mem_stream, {.type = CompressionType::RAW});

        READ_BINARY_PTR(inflate_stream, uncompressed_data.data(), uncompressed_data.size());
    }

    // Lud::check::eq(
    //     lfh.CRC_32,
    //     crc32(0, uncompressed_data.data(), lfh.uncompressed_size),
    //     std::format("File: [{}] is corrupted and can not be recovered", lfh.file_name));

    return uncompressed_data;
}

std::vector<ArchiveEntry> ZipArchive::GetDirectory() const
{
    std::vector<ArchiveEntry> directory;

    const auto& entries = p_impl->file_entries;

    directory.reserve(entries.size());
    for (size_t i = 0; i < entries.size(); i++)
    {
        directory.emplace_back(
            entries[i].header.file_name,
            i,
            entries[i].header.uncompressed_size,
            entries[i].header.compressed_size
        );
    }
    return directory;
}

std::vector<uint8_t> ZipArchive::Pop(const ArchiveEntry& entry)
{
    auto& file_entries = p_impl->file_entries;

    auto& file_entry_to_remove = file_entries[entry.index];

    file_entries.erase(file_entries.begin() + static_cast<ptrdiff_t>(entry.index));

    auto& [lfh, compressed_data] = file_entry_to_remove;
    std::vector<uint8_t> uncompressed_data;
    uncompressed_data.resize(lfh.uncompressed_size);
    if (lfh.compression_method == CompressionMethod::NONE)
    {
        std::copy_n(compressed_data.begin(), lfh.uncompressed_size, uncompressed_data.begin());
    }
    else
    {
        Lud::memory_istream<uint8_t> mem_stream(compressed_data);
        inflate_istream inflate_stream(mem_stream, {.type = CompressionType::RAW});

        READ_BINARY_PTR(inflate_stream, uncompressed_data.data(), uncompressed_data.size());
    }

    return uncompressed_data;
}

void ZipArchive::read(std::istream& stream)
{
    auto eocd_size = static_cast<std::streamoff>(find_eocd_size_from_zip(stream));

    stream.seekg(-eocd_size, std::ios::end);

    auto& entries = p_impl->file_entries;
    auto eocd = read_end_of_central_directory_record(stream);

    stream.seekg(eocd.offset);

    for (size_t i = 0; i < eocd.directory_record_number; i++)
    {
        auto cdh = read_central_directory_header(stream);
        const auto current_pos = stream.tellg();
        stream.seekg(static_cast<std::streamoff>(cdh.offset));

        auto lfh = read_local_file_header(stream);
        std::vector<uint8_t> compressed_data(lfh.compressed_size);
        READ_BINARY_PTR(stream, compressed_data.data(), compressed_data.size());
        stream.seekg(current_pos);

        entries.emplace_back(
            lfh,
            std::move(compressed_data)
        );
    }
}

void ZipArchive::Write(std::ostream& stream) const
{
    const auto& file_entries = p_impl->file_entries;

    std::vector<CentralDirectoryHeader> central_directory;
    central_directory.reserve(file_entries.size());

    uint32_t total_written = 0;
    uint32_t central_directory_size = 0;

    for (const auto& entry : file_entries)
    {
        central_directory.emplace_back(
            Signatures::CENTRAL_DIRECTORY_HEADER,
            0,
            2,
            0,
            entry.header.compression_method,
            0,
            0,
            entry.header.CRC_32,
            entry.header.compressed_size,
            entry.header.uncompressed_size,
            entry.header.file_name_length,
            0,
            0,
            0,
            0,
            0,
            total_written,
            entry.header.file_name
        );

        write_local_file_header(stream, entry.header);
        WRITE_BINARY_PTR(stream, entry.compressed_data.data(), entry.compressed_data.size());

        total_written += get_local_file_header_size(entry.header) + entry.header.compressed_size;
        central_directory_size += get_central_directory_header_size(central_directory.back());
    }
    for (const auto& directory : central_directory)
    {
        write_central_directory_header(stream, directory);
    }

    EndOfCentralDirectoryRecord eocd{
        .signature = Signatures::END_OF_CENTRAL_DIRECTORY_RECORD,
        .disk_number = 0,
        .disk_start_number = 0,
        .directory_record_number_disk = static_cast<uint16_t>(central_directory.size()),
        .directory_record_number = static_cast<uint16_t>(central_directory.size()),
        .central_directory_size = central_directory_size,
        .offset = total_written,
        .comment_length = 0
    };

    write_end_of_central_directory_record(stream, eocd);
}

} // namespace varf::comp
