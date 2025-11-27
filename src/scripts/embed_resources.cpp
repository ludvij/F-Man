#include "archive/rezip.hpp"

#include <ludutils/lud_mem_stream.hpp>

#include <cstdint>

namespace fs = std::filesystem;
static std::vector<std::filesystem::path> traverse(const std::string_view path)
{
    std::vector<fs::path> result;

    std::deque<fs::directory_entry> iters;

    iters.emplace_back(path);
    while (!iters.empty())
    {
        fs::directory_iterator it{iters.front()};
        iters.pop_front();
        for (const auto& dir_entry : it)
        {
            fs::path rel = path / fs::relative(dir_entry, path);
            rel.make_preferred();
            if (fs::is_directory(dir_entry))
            {
                iters.push_back(dir_entry);
            }
            else
            {
                result.emplace_back(rel);
            }
        }
    }
    return result;
}

static void hexdump(std::ostream& output, const std::span<uint8_t> data, unsigned int row_size = 12)
{

    for (size_t i = 0; i < data.size(); i++)
    {
        std::print(output, "0x{:0>2X}", data[i]);
        if (i != data.size() - 1)
        {
            output << ", ";
        }
        if (i % row_size == (row_size - 1) || i == data.size() - 1)
        {
            output << '\n';
        }
    }
}

int main(int argc, char** argv)
{
    const auto resources_path = argc > 1 ? argv[1] : "resources/";
    const auto output_file = argc > 2 ? argv[2] : "generated_resources.cpp";
    const auto var_name = argc > 3 ? argv[3] : "RESOURCES_BINDUMP";

    varf::RezipArchive archive;
    for (const auto& file : traverse(resources_path))
    {
        std::ifstream stream(file, std::ios::binary);
        archive.Push(file.string(), stream);
    }

    std::vector<uint8_t> data;
    Lud::vector_ostream vec_stream(data);
    archive.Write(vec_stream);

    std::ofstream output(output_file);
    output << "unsigned char " << var_name << "[] = {\n";
    hexdump(output, data);
    output << "};\n";
    output << "size_t " << var_name << "_len = " << data.size() << ";";
    return 0;
}
