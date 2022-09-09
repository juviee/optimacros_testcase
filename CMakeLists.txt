cmake_minimum_required(VERSION 3.10.0)
project(how2thread
        VERSION 0.0.1
        DESCRIPTION "simple mt parser app"
        LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set_source_files_properties(src/main.cpp src/how2thread.cpp PROPERTIES LANGUAGE CXX )

set(CMAKE_CXX_COMPILER "g++")
add_executable(how2thread src/main.cpp src/how2thread.cpp)

add_link_options(
    -lpthread
    -lstdc++
)

#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libstdc++")
target_link_libraries(how2thread PRIVATE Threads::Threads)
# target_include_directories(how2thread PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/)