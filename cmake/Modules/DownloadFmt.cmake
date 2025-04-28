include(FetchContent)

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
add_compile_definitions(FMT_HEADER_ONLY)
include_directories(SYSTEM ${fmt_SOURCE_DIR}/include)
set(fmt_POPULATED ${fmt_POPULATED} PARENT_SCOPE)
set(fmt_SOURCE_DIR ${fmt_SOURCE_DIR} PARENT_SCOPE)
