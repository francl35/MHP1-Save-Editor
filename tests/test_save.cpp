#include "mhp1/save.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
int main() {
    auto path=std::filesystem::temp_directory_path()/"mhp1_test_input.bin";
    auto out=std::filesystem::temp_directory_path()/"mhp1_test_output.bin";
    std::vector<char> empty(mhp1::save_size,0);
    { std::ofstream f(path,std::ios::binary); f.write(empty.data(),empty.size()); }
    auto s=mhp1::Save::load(path);
    assert(s.checksum_valid());
    s.set_name(0,mhp1::utf8_to_utf16("あああ"));
    s.set_gender(0,1); s.set_face(0,23); s.set_hair(0,12); s.set_zenny(0,7000);
    s.update_checksum(); assert(s.checksum_valid());
    s.write(out);
    auto t=mhp1::Save::load(out);
    assert(t.bytes().size()==mhp1::save_size);
    auto c=t.character(0);
    assert(mhp1::utf16_to_utf8(c.name)=="あああ");
    assert(c.gender==1 && c.face==23 && c.hair==12 && c.zenny==7000);
    assert(t.bytes()[0x5378]==0x42 && t.bytes()[0x5379]==0x30);
    bool threw=false;
    try { t.set_name(0,u"123456789"); } catch(const std::invalid_argument&) { threw=true; }
    assert(threw);
    t.set_name(0,u"HUNTER7"); t.update_checksum();
    assert(t.checksum_valid());
    assert(t.character(0).name==u"HUNTER7");
    assert(t.bytes()[0x5378+16]==0 && t.bytes()[0x5378+17]==0);
    std::filesystem::remove(path); std::filesystem::remove(out);
    std::cout<<"All MHP1 core tests passed\n";
}
