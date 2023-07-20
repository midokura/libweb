cmake_minimum_required(VERSION 3.13.5)
project(slweb C)
add_library(${PROJECT_NAME}
    handler.c
    html.c
    http.c
    server.c
    wildcard_cmp.c)
add_subdirectory(dynstr)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} PUBLIC dynstr)
