cmake_minimum_required( VERSION 3.19)
include(FetchContent)

include_guard()

project(MpeghUiTranslator VERSION "1.0.0")

set(mpeghuitranslator_BUILD_DOC      OFF CACHE BOOL "Build documentation PDF")
set(mpeghuitranslator_BUILD_BINARIES OFF CACHE BOOL "Build demo executables")

FetchContent_Declare(
  jsoncpp
  URL https://github.com/open-source-parsers/jsoncpp/archive/refs/tags/1.9.5.zip
)

FetchContent_Declare(
  libxml2
  URL https://github.com/GNOME/libxml2/archive/refs/tags/v2.9.12.zip
)

add_subdirectory(src)

if(mpeghuitranslator_BUILD_BINARIES)
  add_subdirectory(demos)
endif()

if(mpeghuitranslator_BUILD_DOC)
  add_subdirectory(doc)
endif()

