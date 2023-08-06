cmake_minimum_required(VERSION 3.13)
project(hello C)
add_executable(hello main.c)
target_link_libraries(${PROJECT_NAME} PRIVATE slweb dynstr)
