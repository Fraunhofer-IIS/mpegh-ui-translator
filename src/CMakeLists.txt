# Configure dependencies to exclude all unnecessary transitive dependencies and optional features
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS_BASE OFF)
if(BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS_BASE ON)
endif()
set(BUILD_SHARED_LIBS OFF)

set(JSONCPP_WITH_TESTS OFF)

set(LIBXML2_WITH_DEBUG OFF)
set(LIBXML2_WITH_HTML OFF)
set(LIBXML2_WITH_HTTP OFF)
set(LIBXML2_WITH_ICONV OFF)
set(LIBXML2_WITH_LZMA OFF)
set(LIBXML2_WITH_MODULES OFF)
set(LIBXML2_WITH_PROGRAMS OFF)
set(LIBXML2_WITH_PYTHON OFF)
set(LIBXML2_WITH_TESTS OFF)
set(LIBXML2_WITH_ZLIB OFF)

FetchContent_MakeAvailable(jsoncpp libxml2)
# Reset static/shared flag for main library
set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_BASE})

add_library(mpeghuitranslator
  audio_scene.h
  json_composer.cpp
  json_parser.cpp
  mpeghuitranslator.cpp
  xml_composer.cpp
  xml_parser.cpp
)
target_include_directories(mpeghuitranslator PRIVATE .)
target_include_directories(mpeghuitranslator PUBLIC ../include/)
target_link_libraries(mpeghuitranslator PUBLIC jsoncpp_static LibXml2)
