cmake_minimum_required(VERSION 3.13)
project(put C)
add_executable(${PROJECT_NAME} main.c)
target_link_libraries(${PROJECT_NAME} PRIVATE web dynstr)
