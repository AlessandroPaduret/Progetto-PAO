cmake_minimum_required(VERSION 3.16)

project(pao_server VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Il server usa POSIX (timegm/gmtime_r): estensioni GNU abilitate
set(CMAKE_CXX_EXTENSIONS ON)

# Niente test di model/db dentro il build del server
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

add_compile_options(-Wall -Wextra)

# Librerie del progetto
add_subdirectory(../model model)
add_subdirectory(../db db)

# Dipendenze: nlohmann via apt; cpp-httplib e jwt-cpp via FetchContent
# (header-only, non presenti in apt su Ubuntu 24.04). Versioni pinnate per
# build riproducibili. Nota: cpp-httplib < 0.19 non compila con GCC 13.2.
find_package(nlohmann_json REQUIRED)

include(FetchContent)
FetchContent_Declare(
    cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib
    GIT_TAG v0.20.0
)
FetchContent_Declare(
    jwt-cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp
    GIT_TAG v0.7.0
)
FetchContent_MakeAvailable(cpp-httplib jwt-cpp)

# Componenti riusabili del server (auth/mappers/time): testabili senza HTTP
add_library(api_core STATIC
    src/auth.cpp
    src/mappers.cpp
    src/iso8601.cpp
)
target_include_directories(api_core PUBLIC src)
target_link_libraries(api_core PUBLIC
    events
    db_repository
    nlohmann_json::nlohmann_json
    jwt-cpp
)

# Eseguibile HTTP
add_executable(api_server src/main.cpp)
target_link_libraries(api_server PRIVATE api_core httplib::httplib)

# --- Test del server (Catch2) ---
set(BUILD_TESTING ON CACHE BOOL "" FORCE)
include(CTest)
find_package(Catch2 3 REQUIRED)
add_executable(server_tests
    tests/test_time.cpp
    tests/test_auth.cpp
    tests/test_mappers.cpp
)
target_link_libraries(server_tests PRIVATE api_core Catch2::Catch2WithMain)
add_test(NAME server_tests COMMAND server_tests)
