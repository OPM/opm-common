include(FetchContent)
FetchContent_Declare(pybind11
                     DOWNLOAD_EXTRACT_TIMESTAMP ON
                     URL https://github.com/pybind/pybind11/archive/refs/tags/v2.11.1.tar.gz
                     URL_HASH SHA512=ed1512ff0bca3bc0a45edc2eb8c77f8286ab9389f6ff1d5cb309be24bc608abbe0df6a7f5cb18c8f80a3bfa509058547c13551c3cd6a759af708fd0cdcdd9e95)
FetchContent_MakeAvailable(pybind11)
