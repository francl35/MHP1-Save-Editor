#include "mhp1/save.hpp"
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace mhp1 {
namespace {
void append_utf8(std::string& s, std::uint32_t cp) {
    if (cp <= 0x7F) s.push_back(static_cast<char>(cp));
    else if (cp <= 0x7FF) { s.push_back(char(0xC0 | (cp >> 6))); s.push_back(char(0x80 | (cp & 63))); }
    else if (cp <= 0xFFFF) { s.push_back(char(0xE0 | (cp >> 12))); s.push_back(char(0x80 | ((cp >> 6) & 63))); s.push_back(char(0x80 | (cp & 63))); }
    else { s.push_back(char(0xF0 | (cp >> 18))); s.push_back(char(0x80 | ((cp >> 12) & 63))); s.push_back(char(0x80 | ((cp >> 6) & 63))); s.push_back(char(0x80 | (cp & 63))); }
}
}
std::u16string utf8_to_utf16(const std::string& s) {
    std::u16string out;
    for (std::size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i++]);
        std::uint32_t cp = 0; int n = 0; std::uint32_t min = 0;
        if (c < 0x80) cp = c;
        else if ((c & 0xE0) == 0xC0) { cp = c & 31; n = 1; min = 0x80; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 15; n = 2; min = 0x800; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 7; n = 3; min = 0x10000; }
        else throw std::invalid_argument("Invalid UTF-8");
        if (i + n > s.size()) throw std::invalid_argument("Truncated UTF-8");
        for (int j = 0; j < n; ++j) {
            auto d = static_cast<unsigned char>(s[i++]);
            if ((d & 0xC0) != 0x80) throw std::invalid_argument("Invalid UTF-8 continuation");
            cp = (cp << 6) | (d & 63);
        }
        if ((n && cp < min) || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
            throw std::invalid_argument("Invalid Unicode scalar");
        if (cp <= 0xFFFF) out.push_back(static_cast<char16_t>(cp));
        else { cp -= 0x10000; out.push_back(static_cast<char16_t>(0xD800 | (cp >> 10))); out.push_back(static_cast<char16_t>(0xDC00 | (cp & 1023))); }
    }
    return out;
}
std::string utf16_to_utf8(const std::u16string& s) {
    std::string out;
    for (std::size_t i = 0; i < s.size(); ++i) {
        std::uint32_t cp = s[i];
        if (cp >= 0xD800 && cp <= 0xDBFF) {
            if (++i >= s.size() || s[i] < 0xDC00 || s[i] > 0xDFFF) throw std::invalid_argument("Invalid UTF-16 surrogate");
            cp = 0x10000 + ((cp - 0xD800) << 10) + (s[i] - 0xDC00);
        } else if (cp >= 0xDC00 && cp <= 0xDFFF) throw std::invalid_argument("Invalid UTF-16 surrogate");
        append_utf8(out, cp);
    }
    return out;
}
Save Save::load(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open input save");
    Save save;
    save.bytes_ = std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), {});
    if (save.bytes_.size() != save_size) throw std::runtime_error("Expected decrypted 0x30000-byte MHPSP.BIN");
    return save;
}
void Save::write(const std::filesystem::path& path) const {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("Cannot open output save");
    out.write(reinterpret_cast<const char*>(bytes_.data()), static_cast<std::streamsize>(bytes_.size()));
    if (!out) throw std::runtime_error("Failed to write save");
}
std::size_t Save::base(std::size_t slot) const {
    if (slot >= slot_count) throw std::out_of_range("Slot must be 0, 1, or 2");
    return slot_base + slot * slot_stride;
}
std::uint32_t Save::read32(std::size_t o) const {
    return std::uint32_t(bytes_.at(o)) | (std::uint32_t(bytes_.at(o+1)) << 8) |
        (std::uint32_t(bytes_.at(o+2)) << 16) | (std::uint32_t(bytes_.at(o+3)) << 24);
}
void Save::write32(std::size_t o, std::uint32_t v) {
    for (int i=0; i<4; ++i) bytes_.at(o+i) = static_cast<std::uint8_t>(v >> (i*8));
}
bool Save::slot_occupied(std::size_t slot) const { return bytes_.at(base(slot)) == 1; }
Character Save::character(std::size_t slot) const {
    const auto b = base(slot);
    Character c;
    c.gender = bytes_.at(b+1); c.face = bytes_.at(b+3); c.hair = bytes_.at(b+4);
    c.zenny = read32(b+0x34);
    for (std::size_t i=0; i<name_capacity_units; ++i) {
        auto p = b+0x1C+i*2;
        auto u = static_cast<char16_t>(bytes_.at(p) | (bytes_.at(p+1) << 8));
        if (!u) break;
        c.name.push_back(u);
    }
    return c;
}
void Save::set_gender(std::size_t slot, std::uint8_t v) { if (v>1) throw std::invalid_argument("Gender must be 0 or 1"); bytes_.at(base(slot)+1)=v; }
void Save::set_face(std::size_t slot, std::uint8_t v) { if (v>23) throw std::invalid_argument("Face must be 1-24"); bytes_.at(base(slot)+3)=v; }
void Save::set_hair(std::size_t slot, std::uint8_t v) { if (v>12) throw std::invalid_argument("Hair must be 1-13"); bytes_.at(base(slot)+4)=v; }
void Save::set_zenny(std::size_t slot, std::uint32_t v) { write32(base(slot)+0x34,v); }
void Save::set_name(std::size_t slot, const std::u16string& name) {
    if (name.empty() || name.size() > name_capacity_units) throw std::invalid_argument("Name must contain 1-8 UTF-16 code units (5-8 experimental)");
    for (char16_t ch : name) if (ch == 0 || (ch >= 0xD800 && ch <= 0xDFFF)) throw std::invalid_argument("Unsupported name code unit");
    const auto b=base(slot);
    for (auto rel : {std::size_t(0x1C), std::size_t(0x4378)}) {
        for (std::size_t i=0; i<=name_capacity_units; ++i) {
            auto ch = i<name.size() ? name[i] : char16_t(0);
            bytes_.at(b+rel+i*2) = static_cast<std::uint8_t>(ch);
            bytes_.at(b+rel+i*2+1) = static_cast<std::uint8_t>(ch >> 8);
        }
    }
}
std::uint32_t Save::checksum() const {
    std::uint32_t sum=0;
    for (std::size_t i=0; i<checksum_offset; i+=4) sum+=read32(i);
    return sum;
}
bool Save::checksum_valid() const { return checksum() == read32(checksum_offset); }
void Save::update_checksum() { write32(checksum_offset,checksum()); }
}
