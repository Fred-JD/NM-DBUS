cmake_minimum_required (VERSION 3.11)
 
# --- Fetch FTXUI --------------------------------------------------------------
include(FetchContent)
 
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  # Important: Specify a GIT_TAG XXXXX here.
)
 
FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
  FetchContent_Populate(ftxui)
  add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
 
# ------------------------------------------------------------------------------
 
project(ftxui-starter
  LANGUAGES CXX
  VERSION 1.0.0
)

find_package(PkgConfig)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0 libnm)

add_executable(ftxui-starter src/main.cpp src/nm_class.cpp)
target_include_directories(ftxui-starter PRIVATE src)
 
target_link_libraries(ftxui-starter
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component # Not needed for this example.
  PkgConfig::deps
)

add_definitions( ${NMLIB_CFLAGS} ${NMLIB_CFLAGS_OTHER} )
