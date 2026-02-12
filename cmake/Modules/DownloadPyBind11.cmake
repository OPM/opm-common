include(FetchContent)

if(Python3_VERSION_MINOR LESS 8)
  FetchContent_Declare(pybind11
                       URL https://github.com/pybind/pybind11/archive/refs/tags/v2.11.1.tar.gz
                       URL_HASH SHA512=ed1512ff0bca3bc0a45edc2eb8c77f8286ab9389f6ff1d5cb309be24bc608abbe0df6a7f5cb18c8f80a3bfa509058547c13551c3cd6a759af708fd0cdcdd9e95)
else()
  FetchContent_Declare(pybind11
                       DOWNLOAD_EXTRACT_TIMESTAMP ON
                       URL https://github.com/pybind/pybind11/archive/refs/tags/v2.13.6.tar.gz
                       URL_HASH SHA512=497c25b33b09a9c42f67131ab82e35d689e8ce089dd7639be997305ff9a6d502447b79c824508c455d559e61f0186335b54dd2771d903a7c1621833930622d1a)
endif()

set(PYBIND11_FINDPYTHON ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(pybind11)
