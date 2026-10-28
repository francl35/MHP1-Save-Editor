#include "mhp1/save.hpp"
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    try {
        if (argc < 3) {
            std::cout << "Usage: mhp1_cli INPUT.BIN OUTPUT.BIN [--slot 0-2] [--name UTF8] [--gender male|female] [--face 1-24] [--hair 1-13] [--zenny N]\n";
            return 1;
        }
        auto input=std::string(argv[1]), output=std::string(argv[2]);
        if (input == output) throw std::invalid_argument("Input and output must differ");
        auto save=mhp1::Save::load(input);
        if (!save.checksum_valid()) throw std::runtime_error("Input checksum invalid; refusing to edit");
        std::size_t slot=0;
        std::optional<std::string> name,gender;
        std::optional<int> face,hair;
        std::optional<std::uint32_t> zenny;
        for (int i=3;i<argc;i+=2) {
            if (i+1>=argc) throw std::invalid_argument("Missing option value");
            std::string opt=argv[i], value=argv[i+1];
            if (opt=="--slot") slot=std::stoul(value);
            else if (opt=="--name") name=value;
            else if (opt=="--gender") gender=value;
            else if (opt=="--face") face=std::stoi(value);
            else if (opt=="--hair") hair=std::stoi(value);
            else if (opt=="--zenny") { auto n=std::stoull(value); if(n>UINT32_MAX) throw std::out_of_range("Zenny overflow"); zenny=static_cast<std::uint32_t>(n); }
            else throw std::invalid_argument("Unknown option: "+opt);
        }
        auto before=save.character(slot);
        if (name) save.set_name(slot,mhp1::utf8_to_utf16(*name));
        if (gender) {
            if (*gender!="male" && *gender!="female") throw std::invalid_argument("Gender must be male or female");
            save.set_gender(slot,*gender=="female" ? 1 : 0);
        }
        if (face) { if (*face<1 || *face>24) throw std::out_of_range("Face range 1-24"); save.set_face(slot,static_cast<std::uint8_t>(*face-1)); }
        if (hair) { if (*hair<1 || *hair>13) throw std::out_of_range("Hair range 1-13"); save.set_hair(slot,static_cast<std::uint8_t>(*hair-1)); }
        if (zenny) save.set_zenny(slot,*zenny);
        save.update_checksum();
        save.write(output);
        auto after=save.character(slot);
        std::cout << "Slot "<<slot<<": "<<mhp1::utf16_to_utf8(before.name)<<" -> "<<mhp1::utf16_to_utf8(after.name)
                  <<" | gender "<<int(after.gender)<<" | face "<<int(after.face)+1<<" | hair "<<int(after.hair)+1
                  <<" | zenny "<<after.zenny<<"\nSaved "<<output<<" (checksum valid: "<<save.checksum_valid()<<")\n";
    } catch(const std::exception& e) { std::cerr<<"Error: "<<e.what()<<'\n'; return 2; }
}
