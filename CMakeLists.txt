cmake_minimum_required(VERSION 3.13.5)
option(BUILD_EXAMPLES "Build examples" ON)
project(web C VERSION 0.1.0)
add_library(${PROJECT_NAME}
    handler.c
    html.c
    http.c
    server.c
    wildcard_cmp.c)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR}/cmake)
find_package(dynstr 0.1.0)

if(NOT DYNSTR_FOUND)
    message(STATUS "Using in-tree dynstr")
    add_subdirectory(dynstr)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} PUBLIC dynstr)
install(TARGETS ${PROJECT_NAME})
install(DIRECTORY include/libweb TYPE INCLUDE)
file(READ ${CMAKE_CURRENT_LIST_DIR}/libweb.pc libweb_pc)
string(REPLACE /usr/local ${CMAKE_INSTALL_PREFIX} libweb_repl_pc ${libweb_pc})
file(WRITE ${CMAKE_BINARY_DIR}/libweb.pc ${libweb_repl_pc})
include(GNUInstallDirs)
install(FILES ${CMAKE_BINARY_DIR}/libweb.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
add_subdirectory(doc)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
