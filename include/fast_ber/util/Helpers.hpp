#include <span>

#include <fstream>

namespace fast_ber
{
void print_to_file(std::span<const uint8_t> data, const std::string& filename)
{
    std::ofstream file(filename);
    file.write(reinterpret_cast<const char*>(data.data()), data.length());
}
} // namespace fast_ber
