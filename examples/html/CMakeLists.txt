cmake_minimum_required(VERSION 3.13)
project(html C)
add_executable(html main.c)
target_link_libraries(${PROJECT_NAME} PRIVATE slweb dynstr)
