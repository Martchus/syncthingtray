cmake_minimum_required(VERSION 2.8.12)

# meta data
set(META_PROJECT_NAME syncthingplasmoid)
set(META_APP_NAME "Syncthing Plasmoid")
set(META_APP_DESCRIPTION "Plasmoid to interact with Syncthing")
set(META_PROJECT_TYPE qtplugin)
set(META_PLUGIN_CATEGORY plasma/applets)
set(META_QT5_VERSION 5.8)

# source files
set(PLASMOID_FILES
    package/contents/ui/CompactRepresentation.qml
    package/contents/ui/FullRepresentation.qml
    package/contents/ui/DirectoriesPage.qml
    package/contents/ui/DevicesPage.qml
    package/contents/ui/DownloadsPage.qml
    package/contents/ui/RecentChangesPage.qml
    package/contents/ui/TopLevelView.qml
    package/contents/ui/TopLevelItem.qml
    package/contents/ui/DetailView.qml
    package/contents/ui/DetailItem.qml
    package/contents/ui/TabButton.qml
    package/contents/ui/ToolTipTrigger.qml
    package/contents/ui/ToolTipView.qml
    package/contents/ui/ToolBar.qml
    package/contents/ui/ToolButton.qml
    package/contents/ui/TinyButton.qml
    package/contents/ui/IconLabel.qml
    package/contents/ui/StatisticsView.qml
    package/contents/ui/main.qml)

# find ECM (required by KF5Plasma)
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_MODULE_PATH})

# find KF5Plasma
find_package(KF5Plasma REQUIRED)

# find c++utilities
find_package(${PACKAGE_NAMESPACE_PREFIX}c++utilities${CONFIGURATION_PACKAGE_SUFFIX} 5.0.0 REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${CPP_UTILITIES_MODULE_DIRS})

# prepare plasmoid package/configuration
set(PLASMOID_PACKAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/package")
file(MAKE_DIRECTORY "${PLASMOID_PACKAGE_DIR}")
set(PLASMOID_CONFIG_TARGET_FILE "${PLASMOID_PACKAGE_DIR}/metadata.desktop")

# make plugin library
add_subdirectory(lib)

# make plasmoid configuration
include(TemplateFinder)
find_template_file("metadata.desktop" "${META_PROJECT_NAME}" PLASMOID_CONFIG_TEMPLATE_FILE)
get_filename_component(PLASMOID_CONFIG_TARGET_FILE_ABSOLUTE_PATH "${PLASMOID_CONFIG_TARGET_FILE}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
configure_file("${PLASMOID_CONFIG_TEMPLATE_FILE}" "${PLASMOID_CONFIG_TARGET_FILE}")

# make plasmoid package folder
foreach (PLASMOID_FILE ${PLASMOID_FILES})
    get_filename_component(PLASMOID_FILE_DIR "${PLASMOID_FILE}" DIRECTORY)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${PLASMOID_FILE_DIR}")
    file(COPY "${PLASMOID_FILE}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${PLASMOID_FILE_DIR}")
endforeach ()

# install plasmoid files
plasma_install_package("${PLASMOID_PACKAGE_DIR}" "${META_ID}")

# add target to ease testing the plasmoid (see testing.md)
set(PLASMOID_TESTDIR "${CMAKE_BINARY_DIR}" CACHE STRING "specifies the Plasmoid test directory")
file(MAKE_DIRECTORY "${PLASMOID_TESTDIR}")
add_custom_target(init_plasmoid_testing
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/inittesting.sh" "${PLASMOID_CONFIG_TARGET_FILE_ABSOLUTE_PATH}"
    WORKING_DIRECTORY "${PLASMOID_TESTDIR}"
)
