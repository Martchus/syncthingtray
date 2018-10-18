cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# metadata
set(META_PROJECT_NAME syncthingconnector)
set(META_PROJECT_TYPE library)
set(META_APP_NAME "Connection backend of Syncthing Tray")
set(META_APP_DESCRIPTION "Connection backend of Syncthing Tray")
set(META_PROJECT_VARNAME_UPPER LIB_SYNCTHING_CONNECTOR)
set(META_PUBLIC_QT_MODULES Core Network)

# add project files
set(HEADER_FILES
    syncthingdir.h
    syncthingdev.h
    syncthingconnection.h
    syncthingconnectionsettings.h
    syncthingnotifier.h
    syncthingconfig.h
    syncthingprocess.h
    syncthingservice.h
    utils.h
)
set(SRC_FILES
    syncthingdir.cpp
    syncthingdev.cpp
    syncthingconnection.cpp
    syncthingconnectionsettings.cpp
    syncthingnotifier.cpp
    syncthingconfig.cpp
    syncthingprocess.cpp
    syncthingservice.cpp
    utils.cpp
)

set(TEST_HEADER_FILES
)
set(TEST_SRC_FILES
    tests/cppunit.cpp
    tests/connectiontests.cpp
    tests/misctests.cpp
)

set(TS_FILES
    translations/${META_PROJECT_NAME}_de_DE.ts
    translations/${META_PROJECT_NAME}_en_US.ts
)

# find c++utilities
find_package(c++utilities 4.15.0 REQUIRED)
use_cpp_utilities()
set(META_PUBLIC_SHARED_LIB_DEPENDS c++utilities)
set(META_PUBLIC_STATIC_LIB_DEPENDS c++utilities_static)

# find qtutilities (only headers and CMake modules used)
find_package(qtutilities 5.0.0 REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${QT_UTILITIES_MODULE_DIRS})
list(APPEND PRIVATE_SHARED_INCLUDE_DIRS ${QT_UTILITIES_INCLUDE_DIRS})
list(APPEND PRIVATE_STATIC_INCLUDE_DIRS ${QT_UTILITIES_INCLUDE_DIRS})

# link also explicitely against the following Qt 5 modules
list(APPEND ADDITIONAL_QT_MODULES Network)
set(META_PUBLIC_QT_MODULES Core ${ADDITIONAL_QT_MODULES})

# configure support for controlling Syncthing via systemd service
if(UNIX AND NOT APPLE)
    set(ENABLE_SYSTEMD_SUPPORT_BY_DEFAULT ON)
else()
    set(ENABLE_SYSTEMD_SUPPORT_BY_DEFAULT OFF)
endif()
option(SYSTEMD_SUPPORT "enables support for controlling Syncthing systemd service" ${ENABLE_SYSTEMD_SUPPORT_BY_DEFAULT})
if(SYSTEMD_SUPPORT)
    list(APPEND DBUS_FILES
        org.freedesktop.DBus.Properties.xml
        org.freedesktop.login1.LoginManager.xml
        org.freedesktop.systemd1.Manager.xml
        org.freedesktop.systemd1.Service.xml
        org.freedesktop.systemd1.Unit.xml
    )
    set_source_files_properties(
        org.freedesktop.systemd1.Manager.xml
        PROPERTIES INCLUDE syncthingservice.h
    )
    list(APPEND META_PUBLIC_COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_SUPPORT_SYSTEMD)
    message(STATUS "systemd support enabled")
else()
    message(STATUS "systemd support disabled")
endif()

# configure whether the connection should be mocked for test purposes
option(SYNCTHING_CONNECTION_MOCKED "enables mocking the SyncthingConnection class so it will provide only some fixed test data" OFF)
if(SYNCTHING_CONNECTION_MOCKED)
    list(APPEND SRC_FILES
        syncthingconnectionmockhelpers.h
        syncthingconnectionmockhelpers.cpp
    )
    set_property(
        SOURCE syncthingconnection.cpp
               syncthingconnectionmockhelpers.h
               syncthingconnectionmockhelpers.cpp
        APPEND PROPERTY COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_CONNECTION_MOCKED
    )
    message(WARNING "SyncthingConnection class will be mocked")
endif()

# configure whether the service should be mocked for test purposes
option(SYNCTHING_SERVICE_MOCKED "enables mocking the SyncthingService class so it will provide some fake status" OFF)
if(SYNCTHING_SERVICE_MOCKED)
    set_property(
        SORUCE syncthingservice.cpp
        APPEND PROPERTY COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_SERVICE_MOCKED
    )
    message(WARNING "SyncthingService class will be mocked")
endif()

# configure whether events should be logged
option(SYNCTHING_CONNECTION_LOG_SYNCTHING_EVENTS "enables logging event data to stdout (enable only for debugging!)" OFF)
if(SYNCTHING_CONNECTION_LOG_SYNCTHING_EVENTS)
    set_property(
        SOURCE syncthingconnection.cpp
        APPEND PROPERTY COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_LOG_SYNCTHING_EVENTS
    )
    message(WARNING "SyncthingConnection class will log event data to stdout")
endif()

# configure whether POSTs should be logged
option(SYNCTHING_CONNECTION_LOG_API_CALLS "enables logging API calls done by the SyncthingConnector (enable only for debugging!)" OFF)
if(SYNCTHING_CONNECTION_LOG_API_CALLS)
    set_property(
        SOURCE syncthingconnection.cpp
        APPEND PROPERTY COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_LOG_API_CALLS
    )
    message(WARNING "SyncthingConnection class will log API calls data to stdout")
endif()

# include modules to apply configuration
include(BasicConfig)
include(QtConfig)
include(WindowsResources)
include(LibraryTarget)

# link tests against test helper (can't use find_package because testhelper target is not
# present at this point)
if(BUILD_SHARED_LIBS)
    list(APPEND TEST_LIBRARIES syncthingtesthelper)
else()
    list(APPEND TEST_LIBRARIES syncthingtesthelper_static)
endif()
include(TestTarget)

include(Doxygen)
include(ConfigHeader)
