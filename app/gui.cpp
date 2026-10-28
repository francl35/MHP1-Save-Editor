#include "mhp1/save.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
namespace fs = std::filesystem;
char input_path[2048]="MHPSP.BIN";
char output_path[2048]="MHPSP_EDITED.BIN";
char name[128]="";
std::unique_ptr<mhp1::Save> save;
int slot=0,gender=0,face=1,hair=1,zenny=0;
bool backup_existing=true, backup_source=true, browser_open=false, show_experimental=false;
fs::path browser_dir=fs::current_path();
std::string status="Drop a decrypted MHPSP.BIN here, or browse for one";
void set_path(char* dest, std::size_t capacity, const fs::path& path) {
    const auto s=path.string();
    if (s.size()>=capacity) throw std::runtime_error("File path too long");
    std::snprintf(dest,capacity,"%s",s.c_str());
}
void refresh() {
    auto c=save->character(static_cast<std::size_t>(slot));
    gender=c.gender; face=c.face+1; hair=c.hair+1;
    zenny=static_cast<int>(std::min<std::uint32_t>(c.zenny,2147483647u));
    auto utf8=mhp1::utf16_to_utf8(c.name);
    std::snprintf(name,sizeof(name),"%s",utf8.c_str());
}
void load_file(const fs::path& path) {
    auto loaded=std::make_unique<mhp1::Save>(mhp1::Save::load(path));
    if (!loaded->checksum_valid()) throw std::runtime_error("Checksum mismatch; refusing edit");
    save=std::move(loaded); slot=0;
    set_path(input_path,sizeof(input_path),path);
    set_path(output_path,sizeof(output_path),path.parent_path()/(path.stem().string()+"_EDITED"+path.extension().string()));
    refresh(); status="Loaded; checksum valid";
}
void drop_callback(GLFWwindow*,int count,const char** paths) {
    if (count<1) return;
    try { load_file(fs::u8path(paths[0])); }
    catch(const std::exception& e) { status=e.what(); }
}
void file_browser() {
    if (!browser_open) return;
    ImGui::OpenPopup("Choose decrypted save");
    if (ImGui::BeginPopupModal("Choose decrypted save",&browser_open,ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Folder: %s",browser_dir.string().c_str());
        if (ImGui::Button("Up")) browser_dir=browser_dir.parent_path();
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) { browser_open=false; ImGui::CloseCurrentPopup(); }
        ImGui::Separator();
        if (ImGui::BeginChild("files",ImVec2(610,310),true)) {
            try {
                std::vector<fs::directory_entry> entries;
                for (const auto& entry:fs::directory_iterator(browser_dir)) {
                    if (entry.is_directory() || (entry.is_regular_file() && (entry.path().extension()==".BIN" || entry.path().extension()==".bin")))
                        entries.push_back(entry);
                }
                std::sort(entries.begin(),entries.end(),[](const auto& a,const auto& b){
                    if(a.is_directory()!=b.is_directory()) return a.is_directory();
                    return a.path().filename().string()<b.path().filename().string();
                });
                for(const auto& entry:entries) {
                    std::string label=(entry.is_directory()?"[DIR] ":"[BIN] ")+entry.path().filename().string();
                    if (ImGui::Selectable(label.c_str(),false,ImGuiSelectableFlags_AllowDoubleClick)) {
                        if (entry.is_directory()) browser_dir=entry.path();
                        else { try { load_file(entry.path()); browser_open=false; ImGui::CloseCurrentPopup(); }
                               catch(const std::exception& e) { status=e.what(); } }
                    }
                }
            } catch(const std::exception& e) { ImGui::TextWrapped("%s",e.what()); }
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    }
}
}
int main() {
    if (!glfwInit()) return 1;
#if defined(__APPLE__)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,2);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
    const char* glsl="#version 150";
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    const char* glsl="#version 330";
#endif
    GLFWwindow* window=glfwCreateWindow(860,650,"MHP1 Save Editor",nullptr,nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSetDropCallback(window,drop_callback);
    glfwSwapInterval(1);
    IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window,true); ImGui_ImplOpenGL3_Init(glsl);
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(10,10),ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(840,630),ImGuiCond_Always);
        ImGui::Begin("MHP1 character editor",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
        ImGui::TextWrapped("Drop a decrypted 0x30000-byte MHPSP.BIN onto this window, or browse for one.");
        ImGui::InputText("Input BIN",input_path,sizeof(input_path));
        ImGui::SameLine();
        if (ImGui::Button("Browse...")) { browser_open=true; try { browser_dir=fs::absolute(fs::path(input_path)).parent_path(); if (!fs::is_directory(browser_dir)) browser_dir=fs::current_path(); } catch(...) { browser_dir=fs::current_path(); } }
        if (ImGui::Button("Load save")) {
            try { load_file(fs::u8path(input_path)); } catch(const std::exception& e) { status=e.what(); }
        }
        ImGui::InputText("Output BIN",output_path,sizeof(output_path));
        ImGui::Checkbox("Back up original input (.bak)",&backup_source);
        ImGui::Checkbox("Back up existing output before exporting (.bak)",&backup_existing);
        ImGui::Separator();
        ImGui::BeginDisabled(!save);
        if (save) {
            for (int i=0;i<3;++i) {
                ImGui::PushID(i);
                bool occupied=save->slot_occupied(static_cast<std::size_t>(i));
                std::string label="Slot "+std::to_string(i)+(occupied?" (occupied)":" (empty/unknown)");
                if (ImGui::RadioButton(label.c_str(),slot==i)) { slot=i; try { refresh(); } catch(const std::exception& e) { status=e.what(); } }
                if (i<2) ImGui::SameLine();
                ImGui::PopID();
            }
            if (!save->slot_occupied(static_cast<std::size_t>(slot))) ImGui::TextWrapped("Warning: selected slot is empty or its occupied marker is unrecognized. Editing is not recommended.");
        }
        ImGui::InputText("Hunter name",name,sizeof(name));
        ImGui::TextWrapped("Names use UTF-16LE code units. 1-4 tested; 5-8 experimental (same count for Latin and Japanese BMP characters).");
        ImGui::Checkbox("Enable experimental 5-8 code-unit names",&show_experimental);
        ImGui::Combo("Gender",&gender,"Male\0Female\0");
        ImGui::SliderInt("Face",&face,1,24);
        ImGui::SliderInt("Hairstyle",&hair,1,13);
        ImGui::InputInt("Zenny",&zenny);
        if (ImGui::Button("Export edited save")) {
            try {
                if (!save) throw std::runtime_error("Load a save first");
                const auto source=fs::u8path(input_path), destination=fs::u8path(output_path);
                if (fs::weakly_canonical(source)==fs::weakly_canonical(destination)) throw std::runtime_error("Input and output paths must differ");
                if (zenny<0) throw std::runtime_error("Zenny cannot be negative");
                if (!save->slot_occupied(static_cast<std::size_t>(slot))) throw std::runtime_error("Refusing to edit an empty/unrecognized character slot");
                auto new_name=mhp1::utf8_to_utf16(name);
                if (!show_experimental && new_name.size()>4) throw std::runtime_error("Enable experimental name lengths for 5-8 UTF-16 units");
                auto edited=*save;
                edited.set_name(slot,new_name);
                edited.set_gender(slot,static_cast<std::uint8_t>(gender));
                edited.set_face(slot,static_cast<std::uint8_t>(face-1));
                edited.set_hair(slot,static_cast<std::uint8_t>(hair-1));
                edited.set_zenny(slot,static_cast<std::uint32_t>(zenny));
                edited.update_checksum();
                if (backup_source) {
                    const auto backup=fs::path(source.string()+".bak");
                    if (!fs::exists(backup)) fs::copy_file(source,backup);
                }
                if (backup_existing && fs::exists(destination)) {
                    fs::copy_file(destination,fs::path(destination.string()+".bak"),fs::copy_options::overwrite_existing);
                }
                edited.write(destination);
                status="Exported; checksum updated";
            } catch(const std::exception& e) { status=e.what(); }
        }
        ImGui::EndDisabled();
        ImGui::Separator(); ImGui::TextWrapped("Status: %s",status.c_str());
        ImGui::TextWrapped("Only decrypted saves supported. Slot occupancy uses a tentative marker; validate slots 1/2 in-game. Keep an untouched original backup.");
        ImGui::End();
        file_browser();
        ImGui::Render();
        int w,h; glfwGetFramebufferSize(window,&w,&h);
        glViewport(0,0,w,h); glClearColor(.10f,.11f,.13f,1.f); glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); glfwSwapBuffers(window);
    }
    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
    glfwDestroyWindow(window); glfwTerminate();
}
