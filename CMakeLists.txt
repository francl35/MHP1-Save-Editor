cmake_minimum_required(VERSION 3.20)
project(mhp1_editor VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(mhp1_save STATIC src/save.cpp)
target_include_directories(mhp1_save PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(mhp1_cli app/main.cpp)
target_link_libraries(mhp1_cli PRIVATE mhp1_save)

enable_testing()
add_executable(mhp1_tests tests/test_save.cpp)
target_link_libraries(mhp1_tests PRIVATE mhp1_save)
add_test(NAME mhp1_tests COMMAND mhp1_tests)

option(MHP1_BUILD_GUI "Build Dear ImGui + GLFW GUI (downloads ImGui when configured)" OFF)
if(MHP1_BUILD_GUI)
  include(FetchContent)
  find_package(OpenGL REQUIRED)
  find_package(glfw3 CONFIG QUIET)
  if(NOT TARGET glfw AND NOT TARGET glfw3)
    FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4 GIT_SHALLOW TRUE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
  endif()
  FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.91.9b GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(imgui)
  add_library(imgui_backend STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
  target_include_directories(imgui_backend PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
  if(TARGET glfw)
    target_link_libraries(imgui_backend PUBLIC glfw OpenGL::GL)
  else()
    target_link_libraries(imgui_backend PUBLIC glfw3 OpenGL::GL)
  endif()
  add_executable(mhp1_gui app/gui.cpp)
  target_link_libraries(mhp1_gui PRIVATE mhp1_save imgui_backend)
endif()
