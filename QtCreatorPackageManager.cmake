if (NOT QT_CREATOR_SKIP_PACKAGE_MANAGER_SETUP)
    message(STATUS "Skipping Qt Creator automatic package manager (skipping Conan and VCPKG) - check QtCreatorPackageManager.cmake for more details")
endif()

set(QT_CREATOR_SKIP_PACKAGE_MANAGER_SETUP ON)
set(QT_CREATOR_SKIP_CONAN_SETUP ON)
set(QT_CREATOR_SKIP_VCPKG_SETUP ON)