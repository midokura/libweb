cmake_minimum_required(VERSION 3.13.5)
option(BUILD_EXAMPLES "Build examples" ON)
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
install(TARGETS ${PROJECT_NAME})
install(DIRECTORY include/slweb TYPE INCLUDE)
file(READ ${CMAKE_CURRENT_LIST_DIR}/slweb.pc slweb_pc)
string(REPLACE /usr/local ${CMAKE_INSTALL_PREFIX} slweb_repl_pc ${slweb_pc})
file(WRITE ${CMAKE_BINARY_DIR}/slweb.pc ${slweb_repl_pc})
include(GNUInstallDirs)
install(FILES ${CMAKE_BINARY_DIR}/slweb.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
