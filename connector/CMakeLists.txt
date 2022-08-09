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
    syncthingcompletion.h
    syncthingdir.h
    syncthingdev.h
    syncthingconnection.h
    syncthingconnectionstatus.h
    syncthingconnectionsettings.h
    syncthingnotifier.h
    syncthingconfig.h
    syncthingprocess.h
    syncthingservice.h
    qstringhash.h
    utils.h)
set(SRC_FILES
    syncthingdir.cpp
    syncthingdev.cpp
    syncthingconnection.cpp
    syncthingconnection_requests.cpp
    syncthingconnectionsettings.cpp
    syncthingnotifier.cpp
    syncthingconfig.cpp
    syncthingprocess.cpp
    syncthingservice.cpp
    utils.cpp)

set(TEST_HEADER_FILES)
set(TEST_SRC_FILES tests/connectiontests.cpp tests/misctests.cpp)

set(TS_FILES translations/${META_PROJECT_NAME}_zh_CN.ts translations/${META_PROJECT_NAME}_cs_CZ.ts
             translations/${META_PROJECT_NAME}_de_DE.ts translations/${META_PROJECT_NAME}_en_US.ts)

# find c++utilities
find_package(${PACKAGE_NAMESPACE_PREFIX}c++utilities${CONFIGURATION_PACKAGE_SUFFIX} 5.10.0 REQUIRED)
use_cpp_utilities(VISIBILITY PUBLIC)

# find qtutilities (only headers and CMake modules used)
find_package(${PACKAGE_NAMESPACE_PREFIX}qtutilities${CONFIGURATION_PACKAGE_SUFFIX_QTUTILITIES} 6.3.0 REQUIRED)
use_qt_utilities(ONLY_HEADERS VISIBILITY PUBLIC)

# find boost libraries
option(USE_BOOST_PROCESS "enables Boost.Process for improved behavior of the launcher" ON)
if (USE_BOOST_PROCESS)
    option(BOOST_STATIC_LINKAGE "${STATIC_LINKAGE}" "link statically against Boost (instead of dynamically)")
    set(Boost_USE_MULTITHREADED ON)
    if (BOOST_STATIC_LINKAGE)
        set(Boost_USE_STATIC_LIBS ON)
    endif ()
    # add Boost::boost target which represents include directory for header-only deps and add Boost::filesystem as it is
    # needed by Boost.Process
    set(BOOST_ARGS REQUIRED)
    list(APPEND BOOST_ARGS COMPONENTS filesystem)
    use_package(TARGET_NAME Boost::boost PACKAGE_NAME Boost PACKAGE_ARGS "${BOOST_ARGS}")
    use_package(TARGET_NAME Boost::filesystem PACKAGE_NAME Boost PACKAGE_ARGS "${BOOST_ARGS}")
    list(APPEND META_PUBLIC_COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_BOOST_PROCESS)
    if (WIN32)
        list(APPEND PRIVATE_LIBRARIES ws2_32) # needed by Boost.Asio
    endif ()
    if (MINGW)
        # workaround https://github.com/boostorg/process/issues/96
        set_property(
            SOURCE syncthingprocess.cpp
            APPEND
            PROPERTY COMPILE_DEFINITIONS BOOST_USE_WINDOWS_H WIN32_LEAN_AND_MEAN)
    endif ()
    # add threading library as it is needed by Boost.Process
    use_package(TARGET_NAME Threads::Threads PACKAGE_NAME Threads PACKAGE_ARGS REQUIRED)
endif ()

# link also explicitly against the following Qt modules
list(APPEND ADDITIONAL_QT_MODULES Network)

# configure support for controlling Syncthing via systemd service
if (UNIX AND NOT APPLE)
    set(ENABLE_SYSTEMD_SUPPORT_BY_DEFAULT ON)
else ()
    set(ENABLE_SYSTEMD_SUPPORT_BY_DEFAULT OFF)
endif ()
option(SYSTEMD_SUPPORT "enables support for controlling Syncthing systemd service" ${ENABLE_SYSTEMD_SUPPORT_BY_DEFAULT})
if (SYSTEMD_SUPPORT)
    list(
        APPEND
        DBUS_FILES
        org.freedesktop.DBus.Properties.xml
        org.freedesktop.login1.LoginManager.xml
        org.freedesktop.systemd1.Manager.xml
        org.freedesktop.systemd1.Service.xml
        org.freedesktop.systemd1.Unit.xml)
    set_source_files_properties(org.freedesktop.systemd1.Manager.xml PROPERTIES INCLUDE syncthingservice.h)
    list(APPEND META_PUBLIC_COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_SUPPORT_SYSTEMD)
    message(STATUS "systemd support enabled")
else ()
    message(STATUS "systemd support disabled")
endif ()

# configure whether the SyncthingConnection class should be mocked for test purposes
option(SYNCTHING_CONNECTION_MOCKED
       "enables mocking the SyncthingConnection class so it will provide only some fixed test data" OFF)
if (SYNCTHING_CONNECTION_MOCKED)
    list(APPEND SRC_FILES syncthingconnectionmockhelpers.h syncthingconnectionmockhelpers.cpp)
    set_property(
        SOURCE syncthingconnection.cpp syncthingconnection_requests.cpp syncthingconnectionmockhelpers.h
               syncthingconnectionmockhelpers.cpp
        APPEND
        PROPERTY COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_CONNECTION_MOCKED)
    message(WARNING "SyncthingConnection class will be mocked")
endif ()

# configure whether the SyncthingService class should be mocked for test purposes
option(SYNCTHING_SERVICE_MOCKED "enables mocking the SyncthingService class so it will provide some fake status" OFF)
if (SYNCTHING_SERVICE_MOCKED)
    set_property(
        SOURCE syncthingservice.cpp
        APPEND
        PROPERTY COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_SERVICE_MOCKED)
    message(WARNING "SyncthingService class will be mocked")
endif ()

# include modules to apply configuration
include(BasicConfig)
include(QtConfig)
include(WindowsResources)
include(LibraryTarget)

# link tests against test helper (can't use find_package because testhelper target is not present at this point)
list(APPEND TEST_LIBRARIES syncthingtesthelper)
include(TestTarget)

# avoid running this test in parallel with other tests spawning a Syncthing test instance
if (META_MAIN_TEST_NAME)
    set_tests_properties("${META_MAIN_TEST_NAME}" PROPERTIES RESOURCE_LOCK "syncthingtestinstance")
endif ()

include(Doxygen)
include(ConfigHeader)
