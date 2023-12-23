cmake_minimum_required(VERSION 3.26)
project(Lab6)

set(CMAKE_CXX_STANDARD 20)

find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(Lab6 main.cpp
        uuid.hpp
        uuid.hpp
        fs.hpp)

target_link_libraries(Lab6 PRIVATE fmt::fmt)
target_link_libraries(Lab6 PRIVATE nlohmann_json::nlohmann_json)