cmake_minimum_required(VERSION 3.16)

project(pao_client VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

# Qt6 (Widgets per la GUI, Network per le chiamate asincrone all'API)
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(Catch2 3 REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Applicazione desktop
add_executable(pao_client
    src/main.cpp
    src/api/dto.cpp
    src/api/ApiClient.cpp
    src/api/ApiConfig.cpp
    src/controllers/AuthController.cpp
    src/controllers/EventsController.cpp
    src/views/LoginDialog.cpp
    src/views/MainWindow.cpp
    src/views/EventDialog.cpp
    src/views/WeekView.cpp
)
target_include_directories(pao_client PRIVATE src)
target_link_libraries(pao_client PRIVATE Qt6::Widgets Qt6::Network)

# Test dei DTO (headless, solo Qt Core + Catch2)
include(CTest)
add_executable(client_tests
    tests/test_dto.cpp
    src/api/dto.cpp
)
target_include_directories(client_tests PRIVATE src)
target_link_libraries(client_tests PRIVATE Qt6::Core Catch2::Catch2WithMain)
add_test(NAME client_tests COMMAND client_tests)

# Smoke contro un server live (non in ctest: richiede l'API in esecuzione)
add_executable(api_smoke
    tests/api_smoke.cpp
    src/api/dto.cpp
    src/api/ApiClient.cpp
    src/api/ApiConfig.cpp
)
target_include_directories(api_smoke PRIVATE src)
target_link_libraries(api_smoke PRIVATE Qt6::Core Qt6::Network)
