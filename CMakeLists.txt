cmake_minimum_required(VERSION 3.24)
project(run_hidden LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(run_hidden WIN32 src/main.cpp)
target_compile_definitions(run_hidden PRIVATE UNICODE _UNICODE)

enable_testing()
add_test(NAME start_gpg_agent COMMAND run_hidden "gpg-connect-agent" "/bye")

install(TARGETS run_hidden)
install(FILES LICENSE README.md DESTINATION .)

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_DIRECTORY ${PROJECT_BINARY_DIR}/package)
set(CPACK_PACKAGE_CHECKSUM SHA256)
include(CPack)
