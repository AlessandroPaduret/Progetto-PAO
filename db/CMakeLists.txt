cmake_minimum_required(VERSION 3.16)

project(pao_db VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

# Dipendenze dichiarate e risolte dall'ambiente riproducibile (Docker/Ubuntu):
#   - libpqxx  (libreria C++ per PostgreSQL)   -> pkg-config libpqxx
#   - libcrypt (implementazione bcrypt)        -> -lcrypt
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED IMPORTED_TARGET libpqxx)

add_library(db_repository STATIC
    src/db/ConnectionPool.cpp
    src/db/PasswordHasher.cpp
    src/db/UserRepository.cpp
    src/db/EventRepository.cpp
)
target_include_directories(db_repository PUBLIC include)
target_link_libraries(db_repository PUBLIC PkgConfig::PQXX crypt)

# Smoke test: bcrypt senza DB (eseguibile anche senza postgres)
include(CTest)
if(BUILD_TESTING)
    add_executable(hash_smoke tests/hash_smoke.cpp)
    target_link_libraries(hash_smoke PRIVATE db_repository)
    add_test(NAME hash_smoke COMMAND hash_smoke)

    # Smoke test con DB reale: richiede una PostgreSQL raggiungibile
    # via variabile d'ambiente DATABASE_URL (non registrato in ctest).
    add_executable(repo_smoke tests/repo_smoke.cpp)
    target_link_libraries(repo_smoke PRIVATE db_repository)
endif()
