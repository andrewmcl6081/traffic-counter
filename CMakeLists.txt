project (program3)
cmake_minimum_required(VERSION 3.15)

find_package(OpenCV REQUIRED)

set(SOURCES program3.cpp)
add_executable(program3 ${SOURCES})
target_link_libraries(program3 ${OpenCV_LIBS})