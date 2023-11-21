include(FetchContent)
FetchContent_Declare(fmt
                     DOWNLOAD_EXTRACT_TIMESTAMP ON
                     URL https://github.com/fmtlib/fmt/archive/refs/tags/10.1.1.tar.gz
                     URL_HASH SHA512=288c349baac5f96f527d5b1bed0fa5f031aa509b4526560c684281388e91909a280c3262a2474d963b5d1bf7064b1c9930c6677fe54a0d8f86982d063296a54c)
FetchContent_MakeAvailable(fmt)

# We do not want to use the target directly as that means it ends up
# in our depends list for downstream modules. Instead, use header
# only mode.
get_target_property(fmt_inc fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
add_compile_definitions(FMT_HEADER_ONLY)
include_directories(${fmt_inc})
