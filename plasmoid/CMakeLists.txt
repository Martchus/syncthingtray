cmake_minimum_required(VERSION 2.8.12)

# meta data
set(META_PROJECT_NAME syncthingplasmoid)
set(META_APP_NAME "Syncthing Plasmoid")
set(META_APP_AUTHOR "Martchus")
set(META_APP_DESCRIPTION "Plasmoid to interact with Syncthing")
set(META_PROJECT_TYPE qtplugin)
set(META_ID "martchus.${META_PROJECT_NAME}")

# find ECM (required by KF5Plasma)
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_MODULE_PATH})
# find KF5Plasma
find_package(KF5Plasma REQUIRED)

# add subdirs
add_subdirectory(lib)

plasma_install_package(package ${META_ID})

set(PLASMOID_TESTDIR "${CMAKE_CURRENT_BINARY_DIR}/testdir" CACHE STRING "specifies the Plasmoid test directory")
file(MAKE_DIRECTORY "${PLASMOID_TESTDIR}")
add_custom_target(init_plasmoid_testing
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/inittesting.sh"
    WORKING_DIRECTORY "${PLASMOID_TESTDIR}"
)
