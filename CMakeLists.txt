cmake_minimum_required(VERSION 3.20)
project(duck_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# SDL2 from system (apt)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

# glad, glm, stb from vcpkg
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${Stb_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    PkgConfig::SDL2
    glad::glad
    glm::glm
)

# ECS 單元測試（不依賴 OpenGL/SDL2，純 CPU 邏輯）
add_executable(test_ecs tests/test_ecs.cpp)
target_include_directories(test_ecs PRIVATE ${CMAKE_SOURCE_DIR}/src)
