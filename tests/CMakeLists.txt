cmake_minimum_required(VERSION 3.24)
project(WorldDLTests LANGUAGES C CXX)

include(FetchContent)
set(LEVELDB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LEVELDB_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(LEVELDB_INSTALL OFF CACHE BOOL "" FORCE)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
FetchContent_Declare(leveldb
    GIT_REPOSITORY https://github.com/google/leveldb.git
    GIT_TAG 1.23
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(leveldb)

FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(worlddl-tests WorldWriterTests.cpp ../src/world/WorldWriter.cpp)
target_compile_features(worlddl-tests PRIVATE cxx_std_20)
target_include_directories(worlddl-tests PRIVATE ../src)
target_link_libraries(worlddl-tests PRIVATE leveldb)
if(MSVC)
    target_compile_options(worlddl-tests PRIVATE /W4 /utf-8)
endif()
add_executable(worlddl-generation-tests WorldGenerationTests.cpp)
target_compile_features(worlddl-generation-tests PRIVATE cxx_std_20)
target_include_directories(worlddl-generation-tests PRIVATE ../src)
target_link_libraries(worlddl-generation-tests PRIVATE nlohmann_json::nlohmann_json)
if(MSVC)
    target_compile_options(worlddl-generation-tests PRIVATE /W4 /utf-8)
endif()

enable_testing()
add_test(NAME worlddl-storage COMMAND worlddl-tests)
add_test(NAME worlddl-generation COMMAND worlddl-generation-tests)
