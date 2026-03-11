include(FetchContent)

macro(DownloadFmt target)
  if(NOT fmt_POPULATED)
    FetchContent_Declare(fmt
                         DOWNLOAD_EXTRACT_TIMESTAMP ON
                         URL https://github.com/fmtlib/fmt/archive/refs/tags/10.1.1.tar.gz
                         URL_HASH SHA512=288c349baac5f96f527d5b1bed0fa5f031aa509b4526560c684281388e91909a280c3262a2474d963b5d1bf7064b1c9930c6677fe54a0d8f86982d063296a54c)
    FetchContent_Populate(fmt)
  endif()

  # We do not want to use the target directly as that means it ends up
  # in our depends list for downstream modules and the installation list.
  # Instead, we just download and use header only mode.
  target_compile_definitions(${target} PUBLIC $<BUILD_INTERFACE:FMT_HEADER_ONLY>)
  target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${fmt_SOURCE_DIR}/include>)

  # Target used by binaries
  add_library(fmt::fmt INTERFACE IMPORTED)
  target_compile_definitions(fmt::fmt INTERFACE FMT_HEADER_ONLY)
  target_include_directories(fmt::fmt INTERFACE ${fmt_SOURCE_DIR}/include)

  # Required for super-build
  if(NOT PROJECT_IS_TOP_LEVEL)
    set(fmt_POPULATED ${fmt_POPULATED} PARENT_SCOPE)
    set(fmt_SOURCE_DIR ${fmt_SOURCE_DIR} PARENT_SCOPE)
  endif()
endmacro()
