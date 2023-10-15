mark_as_advanced(DYNSTR_LIBRARY DYNSTR_INCLUDE_DIR)
find_library(DYNSTR_LIBRARY NAMES dynstr)
find_path(DYNSTR_INCLUDE_DIR NAMES dynstr PATH_SUFFIXES include)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(dynstr
    DEFAULT_MSG DYNSTR_LIBRARY DYNSTR_INCLUDE_DIR)

if(DYNSTR_FOUND)
    if(NOT TARGET dynstr)
        add_library(dynstr UNKNOWN IMPORTED)
        set_target_properties(dynstr PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${DYNSTR_INCLUDE_DIR}"
            IMPORTED_LOCATION "${DYNSTR_LIBRARY}")
    endif()
endif()
