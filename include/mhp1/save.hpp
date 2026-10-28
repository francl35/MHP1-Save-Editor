#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace mhp1 {
constexpr std::size_t save_size = 0x30000;
constexpr std::size_t checksum_offset = 0x2FFF4;
constexpr std::size_t slot_base = 0x1000;
constexpr std::size_t slot_stride = 0x5000;
constexpr std::size_t slot_count = 3; // layout-derived; validate slots against actual game data
constexpr std::size_t name_capacity_units = 8; // EXPERIMENTAL above 4; second copy has 9 zero UTF-16 units before other data

struct Character {
    std::uint8_t gender{};
    std::uint8_t face{};  // zero-based
    std::uint8_t hair{};  // zero-based
    std::uint32_t zenny{};
    std::u16string name;
};

class Save {
public:
    static Save load(const std::filesystem::path& path);
    void write(const std::filesystem::path& path) const;
    Character character(std::size_t slot) const;
    bool slot_occupied(std::size_t slot) const; // heuristic: first byte == 1
    void set_gender(std::size_t slot, std::uint8_t value);
    void set_face(std::size_t slot, std::uint8_t zero_based);
    void set_hair(std::size_t slot, std::uint8_t zero_based);
    void set_name(std::size_t slot, const std::u16string& name);
    void set_zenny(std::size_t slot, std::uint32_t value);
    std::uint32_t checksum() const;
    bool checksum_valid() const;
    void update_checksum();
    const std::vector<std::uint8_t>& bytes() const { return bytes_; }
private:
    std::vector<std::uint8_t> bytes_;
    std::size_t base(std::size_t slot) const;
    std::uint32_t read32(std::size_t offset) const;
    void write32(std::size_t offset, std::uint32_t value);
};
std::u16string utf8_to_utf16(const std::string& text);
std::string utf16_to_utf8(const std::u16string& text);
}
