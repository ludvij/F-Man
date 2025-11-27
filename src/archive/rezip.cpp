#include "archive/rezip.hpp"

#include <comp_streams/CompStreams.hpp>

#define READ_BINARY_PTR(stream, ptr, sz) stream.read(reinterpret_cast<char*>(ptr), (sz))
#define READ_BINARY(stream, var) READ_BINARY_PTR((stream), &(var), sizeof(var))

#define WRITE_BINARY_PTR(stream, ptr, sz) stream.write(reinterpret_cast<const char*>(ptr), (sz))
#define WRITE_BINARY(stream, var) WRITE_BINARY_PTR((stream), &(var), sizeof(var))

namespace varf {
namespace {

namespace signatures_NS {
enum Enum : uint32_t
{
    LOCAL_FILE_HEADER = 0x0405564C,
    CENTRAL_DIRECTORY_HEADER = 0x0201564C,
    END_OF_CENTRAL_DIRECTORY_RECORD = 0x0605564C,
};
}; // namespace signatures_NS

namespace compression_method_NS {
enum Enum : uint8_t // NOLINT
{
    DEFLATE = 8,
    NONE = 0,
};
}; // namespace compression_method_NS

using Signatures = signatures_NS::Enum;
using CompressionMethod = compression_method_NS::Enum;

struct LocalFileHeader
{
    uint64_t compressed_size{};
    uint64_t uncompressed_size{};
    uint32_t signature{};
    uint32_t CRC_32{};
    uint8_t compression_method{};
};
struct CentralDirectoryHeader
{
    std::string file_name{};
    uint64_t offset{};
    uint32_t signature{};
    uint32_t file_name_length{};
};
struct EndOfCentralDirectoryRecord
{
    uint64_t offset{};
    uint64_t central_directory_size{};
    uint32_t signature{};
    uint32_t directory_record_number{};
};

} // namespace

static constexpr size_t get_local_file_header_size()
{
    return 25UL;
}

static void write_local_file_header(std::ostream& stream, const LocalFileHeader& lfh)
{
    WRITE_BINARY(stream, lfh.signature);
    WRITE_BINARY(stream, lfh.CRC_32);
    WRITE_BINARY(stream, lfh.compressed_size);
    WRITE_BINARY(stream, lfh.uncompressed_size);
    WRITE_BINARY(stream, lfh.compression_method);
}

static LocalFileHeader read_local_file_header(std::istream& stream)
{
    LocalFileHeader lfh;
    READ_BINARY(stream, lfh.signature);
    READ_BINARY(stream, lfh.CRC_32);
    READ_BINARY(stream, lfh.compressed_size);
    READ_BINARY(stream, lfh.uncompressed_size);
    READ_BINARY(stream, lfh.compression_method);

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

static constexpr size_t get_central_directory_header_size(const CentralDirectoryHeader& cdh)
{
    return 16UL + cdh.file_name_length;
}

static void write_central_directory_header(std::ostream& stream, const CentralDirectoryHeader& cdh)
{
    WRITE_BINARY(stream, cdh.signature);
    WRITE_BINARY(stream, cdh.offset);
    WRITE_BINARY(stream, cdh.file_name_length);

    if (cdh.file_name_length > 0)
    {
        WRITE_BINARY_PTR(stream, cdh.file_name.data(), cdh.file_name_length);
    }
}

static CentralDirectoryHeader read_central_directory_header(std::istream& stream)
{
    CentralDirectoryHeader cdh;

    READ_BINARY(stream, cdh.signature);
    READ_BINARY(stream, cdh.offset);
    READ_BINARY(stream, cdh.file_name_length);

    if (cdh.file_name_length > 0)
    {
        cdh.file_name.resize(cdh.file_name_length);
        READ_BINARY_PTR(stream, cdh.file_name.data(), cdh.file_name_length);
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

static constexpr size_t get_end_of_central_directory_record_size()
{
    return 24UL;
}

static void write_end_of_central_directory_record(std::ostream& stream, const EndOfCentralDirectoryRecord& eocd)
{
    WRITE_BINARY(stream, eocd.signature);
    WRITE_BINARY(stream, eocd.central_directory_size);
    WRITE_BINARY(stream, eocd.directory_record_number);
    WRITE_BINARY(stream, eocd.offset);
}

static EndOfCentralDirectoryRecord read_end_of_central_directory_record(std::istream& stream)
{
    const size_t begin_size = stream.tellg();
    EndOfCentralDirectoryRecord eocd;

    READ_BINARY(stream, eocd.signature);
    READ_BINARY(stream, eocd.central_directory_size);
    READ_BINARY(stream, eocd.directory_record_number);
    READ_BINARY(stream, eocd.offset);

    // further validation
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

static std::vector<uint8_t> slurp(std::istream& stream)
{
    const auto current_pos = stream.tellg();
    stream.seekg(0, std::ios::end);
    std::vector<uint8_t> data(stream.tellg());
    stream.seekg(current_pos, std::ios::beg);
    READ_BINARY_PTR(stream, data.data(), data.size());

    return data;
}

struct RezipArchive::Impl
{
    struct file_entry
    {
        LocalFileHeader header;
        std::string name;
        std::vector<uint8_t> compressed_data;
    };
    std::vector<file_entry> file_entries;
};

RezipArchive::RezipArchive()
    : p_impl(new Impl)
{
}

RezipArchive::RezipArchive(std::istream& stream)
    : RezipArchive()
{
    read(stream);
}

RezipArchive::~RezipArchive()
{
    delete p_impl;
}

void RezipArchive::Push(const std::string_view name, std::istream& stream)
{
    auto uncompressed_data = slurp(stream);

    p_impl->file_entries.emplace_back();

    auto& [lfh, file_name, compressed_data] = p_impl->file_entries.back();

    // TODO: implement crc32
    uint32_t crc = 0; // crc32(0, uncompressed_data.data(), uncompressed_data.size());

    if (!uncompressed_data.empty())
    {
        // zlib recommends to set the buffer size to at least the uncompressed size
        compressed_data.reserve(uncompressed_data.size());

        // scoped so sync is automatically called on destruction
        {
            Lud::vector_ostream vec_ostream(compressed_data);
            Lud::deflate_ostream comp_ostream(vec_ostream, {.type = Lud::CompressionType::RAW});

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
    lfh.CRC_32 = crc;
    file_name = name;
}

std::vector<ArchiveEntry> RezipArchive::GetDirectory() const
{
    std::vector<ArchiveEntry> directory;

    const auto& entries = p_impl->file_entries;

    directory.reserve(entries.size());
    for (size_t i = 0; i < entries.size(); i++)
    {
        directory.emplace_back(
            entries[i].name,
            i,
            static_cast<uint32_t>(entries[i].header.uncompressed_size),
            static_cast<uint32_t>(entries[i].header.compressed_size)
        );
    }
    return directory;
}

void RezipArchive::Write(std::ostream& stream) const
{
    const auto& file_entries = p_impl->file_entries;

    std::vector<CentralDirectoryHeader> central_directory;
    central_directory.reserve(file_entries.size());

    uint64_t total_written = 0;
    uint64_t central_directory_size = 0;

    for (const auto& entry : file_entries)
    {
        central_directory.emplace_back(
            entry.name,
            total_written,
            Signatures::CENTRAL_DIRECTORY_HEADER,
            static_cast<uint32_t>(entry.name.size())
        );

        write_local_file_header(stream, entry.header);
        WRITE_BINARY_PTR(stream, entry.compressed_data.data(), entry.compressed_data.size());

        total_written += get_local_file_header_size() + entry.header.compressed_size;
        central_directory_size += get_central_directory_header_size(central_directory.back());
    }
    for (const auto& directory : central_directory)
    {
        write_central_directory_header(stream, directory);
    }

    EndOfCentralDirectoryRecord eocd{
        .offset = total_written,
        .central_directory_size = central_directory_size,
        .signature = Signatures::END_OF_CENTRAL_DIRECTORY_RECORD,
        .directory_record_number = static_cast<uint32_t>(central_directory.size()),
    };

    write_end_of_central_directory_record(stream, eocd);
}

std::vector<uint8_t> RezipArchive::Peek(const ArchiveEntry& entry) const
{
    auto& entries = p_impl->file_entries;

    auto& [lfh, _, compressed_data] = entries[entry.index];
    std::vector<uint8_t> uncompressed_data;
    uncompressed_data.resize(lfh.uncompressed_size);
    if (lfh.compression_method == CompressionMethod::NONE)
    {
        std::copy_n(compressed_data.begin(), lfh.uncompressed_size, uncompressed_data.begin());
    }
    else
    {
        Lud::memory_istream<uint8_t> mem_stream(compressed_data);
        Lud::inflate_istream inflate_stream(mem_stream, {.type = Lud::CompressionType::RAW});

        READ_BINARY_PTR(inflate_stream, uncompressed_data.data(), uncompressed_data.size());
    }

    // Lud::check::eq(
    //     lfh.CRC_32,
    //     crc32(0, uncompressed_data.data(), lfh.uncompressed_size),
    //     std::format("File: [{}] is corrupted and can not be recovered", lfh.file_name));

    return uncompressed_data;
}

std::vector<uint8_t> RezipArchive::Pop(const ArchiveEntry& entry)
{
    auto& file_entries = p_impl->file_entries;

    auto& file_entry_to_remove = file_entries[entry.index];

    file_entries.erase(file_entries.begin() + static_cast<ptrdiff_t>(entry.index));

    const auto& [lfh, name, compressed_data] = file_entry_to_remove;
    std::vector<uint8_t> uncompressed_data;
    uncompressed_data.resize(lfh.uncompressed_size);
    if (lfh.compression_method == CompressionMethod::NONE)
    {
        std::copy_n(compressed_data.begin(), lfh.uncompressed_size, uncompressed_data.begin());
    }
    else
    {
        Lud::memory_istream<uint8_t> mem_stream(compressed_data);
        Lud::inflate_istream inflate_stream(mem_stream, {.type = Lud::CompressionType::RAW});

        READ_BINARY_PTR(inflate_stream, uncompressed_data.data(), uncompressed_data.size());
    }

    return uncompressed_data;
}

void RezipArchive::read(std::istream& stream)
{
    auto eocd_size = static_cast<std::streamoff>(get_end_of_central_directory_record_size());

    stream.seekg(-eocd_size, std::ios::end);

    auto eocd = read_end_of_central_directory_record(stream);

    std::vector<CentralDirectoryHeader> central_directory(eocd.directory_record_number);

    stream.seekg(static_cast<std::streamoff>(eocd.offset), std::ios::beg);

    auto& entries = p_impl->file_entries;

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
            std::move(cdh.file_name),
            std::move(compressed_data)
        );
    }
}

} // namespace varf
