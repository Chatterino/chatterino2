include(FindPackageHandleStandardArgs)

find_path(BoostCertify_INCLUDE_DIR boost/certify/https_verification.hpp HINTS ${CMAKE_SOURCE_DIR}/lib/certify/include)

find_package_handle_standard_args(BoostCertify DEFAULT_MSG BoostCertify_INCLUDE_DIR)

if (BoostCertify_FOUND)
    add_library(BoostCertify INTERFACE IMPORTED)
    set_target_properties(BoostCertify PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${BoostCertify_INCLUDE_DIR}"
            )
endif ()

mark_as_advanced(BoostCertify_INCLUDE_DIR)
