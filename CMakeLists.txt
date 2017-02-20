cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# metadata
set(META_PROJECT_NAME syncthingtray)
set(META_PROJECT_TYPE application)
set(META_APP_AUTHOR "Martchus")
set(META_APP_URL "https://github.com/${META_APP_AUTHOR}/${META_PROJECT_NAME}")
set(META_APP_DESCRIPTION "Tray application for Syncthing")
set(META_APP_CATEGORIES "System;Utility;Network;FileTransfer")
set(META_VERSION_MAJOR 0)
set(META_VERSION_MINOR 4)
set(META_VERSION_PATCH 2)
set(META_VERSION_EXACT_SONAME ON)

project(${META_PROJECT_NAME})

# options for partial build
option(NO_CLI "specifies whether building CLI should be skipped" OFF)
option(NO_TRAY "specifies whether building the tray should be skipped" OFF)
option(NO_FILE_ITEM_ACTION_PLUGIN "specifies whether building the file item action plugin should be skipped" OFF)
option(NO_MODEL "specifies whether building models should be skipped, implies NO_TRAY" OFF)

# add subdirectories
add_subdirectory(connector)
link_directories(${LIB_SYNCTHING_CONNECTOR_BINARY_DIR})
if(NOT NO_CLI)
    add_subdirectory(cli)
endif()
if(NOT NO_MODEL)
    add_subdirectory(model)
    link_directories(${LIB_SYNCTHING_MODEL_BINARY_DIR})
    if(NOT NO_TRAY)
        add_subdirectory(tray)
    endif()
endif()
if(NOT NO_FILE_ITEM_ACTION_PLUGIN)
    add_subdirectory(fileitemactionplugin)
endif()
