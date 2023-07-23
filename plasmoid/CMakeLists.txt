cmake_minimum_required(VERSION 3.17.0)

# meta data
set(META_PROJECT_NAME syncthingplasmoid)
set(META_APP_NAME "Syncthing Plasmoid")
set(META_APP_DESCRIPTION "Plasmoid to interact with Syncthing")
set(META_PROJECT_TYPE qtplugin)
set(META_PLUGIN_CATEGORY plasma/applets)
set(META_QT5_VERSION 5.8)

# use testfiles directory from syncthingconnector
set(META_SRCDIR_REFS "${CMAKE_CURRENT_SOURCE_DIR}\n${CMAKE_CURRENT_SOURCE_DIR}/../syncthingconnector")

# find Plasma
find_package(${KF_PACKAGE_PREFIX}Plasma REQUIRED)

# define list of QML files; there is a distinct directory for each major Plasma version
set(PLASMOID_FILE_NAMES
    CompactRepresentation.qml
    FullRepresentation.qml
    DirectoriesPage.qml
    DevicesPage.qml
    DownloadsPage.qml
    RecentChangesPage.qml
    TopLevelView.qml
    TopLevelItem.qml
    DetailView.qml
    DetailItem.qml
    TabButton.qml
    ToolTipTrigger.qml
    ToolTipView.qml
    ToolBar.qml
    ToolButton.qml
    TinyButton.qml
    IconLabel.qml
    StatisticsView.qml
    main.qml)
set(PLASMOID_FILES_BASE "package${KF_MAJOR_VERSION}")
set(PLASMOID_FILES_UI "${PLASMOID_FILES_BASE}/contents/ui")
foreach (PLASMOID_FILE_NAME ${PLASMOID_FILE_NAMES})
    list(APPEND PLASMOID_FILES "${PLASMOID_FILES_UI}/${PLASMOID_FILE_NAME}")
endforeach ()
file(GLOB_RECURSE PLASMOID_EXCLUDED_FILES LIST_DIRECTORIES false RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "package*/*.qml")
list(REMOVE_ITEM PLASMOID_EXCLUDED_FILES ${PLASMOID_FILES})

# find ECM (required by Plasma)
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_MODULE_PATH})

# find c++utilities
find_package(${PACKAGE_NAMESPACE_PREFIX}c++utilities${CONFIGURATION_PACKAGE_SUFFIX} 5.20.0 REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${CPP_UTILITIES_MODULE_DIRS})

# prepare plasmoid package/configuration
set(PLASMOID_PACKAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/package")
file(MAKE_DIRECTORY "${PLASMOID_PACKAGE_DIR}")
if (KF_MAJOR_VERSION LESS 6)
    set(PLASMOID_CONFIG_TARGET_FILE "${PLASMOID_PACKAGE_DIR}/metadata.desktop")
else ()
    set(PLASMOID_CONFIG_TARGET_FILE "${PLASMOID_PACKAGE_DIR}/metadata.json")
endif ()

# make plugin library
add_subdirectory(lib)

# make plasmoid configuration
include(TemplateFinder)
if (KF_MAJOR_VERSION LESS 6)
    find_template_file("metadata.desktop" "${META_PROJECT_NAME}" PLASMOID_CONFIG_TEMPLATE_FILE)
else ()
    find_template_file("metadata.json" "${META_PROJECT_NAME}" PLASMOID_CONFIG_TEMPLATE_FILE)
endif ()
get_filename_component(PLASMOID_CONFIG_TARGET_FILE_ABSOLUTE_PATH "${PLASMOID_CONFIG_TARGET_FILE}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
configure_file("${PLASMOID_CONFIG_TEMPLATE_FILE}" "${PLASMOID_CONFIG_TARGET_FILE}")

# copy plasmoid package folder plus metadata file into build directory and install it from there
foreach (PLASMOID_FILE ${PLASMOID_FILES})
    get_filename_component(PLASMOID_FILE_DIR "${PLASMOID_FILE}" DIRECTORY)
    string(REGEX REPLACE "package[0-9]+" "package" PLASMOID_FILE_DIR "${PLASMOID_FILE_DIR}")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${PLASMOID_FILE_DIR}")
    file(COPY "${PLASMOID_FILE}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${PLASMOID_FILE_DIR}")
endforeach ()
plasma_install_package("${PLASMOID_PACKAGE_DIR}" "${META_ID}")

# add target to ease testing the plasmoid - using files from source directory directly instead of
# copy in build directory so one does not need to run CMake again to apply changes (see testing.md)
set(PLASMOID_TESTDIR "${CMAKE_BINARY_DIR}" CACHE STRING "specifies the Plasmoid test directory")
file(MAKE_DIRECTORY "${PLASMOID_TESTDIR}")
add_custom_target(init_plasmoid_testing
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/inittesting.sh" "${PLASMOID_CONFIG_TARGET_FILE_ABSOLUTE_PATH}" "${PLASMOID_FILES_BASE}"
    WORKING_DIRECTORY "${PLASMOID_TESTDIR}"
)
