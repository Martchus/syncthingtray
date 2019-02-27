# source files
set(HEADER_FILES syncthingapplet.h settingsdialog.h)
set(SRC_FILES syncthingapplet.cpp settingsdialog.cpp)
set(WIDGETS_UI_FILES appearanceoptionpage.ui)
set(PLASMOID_FILES
    ../package/metadata.desktop
    ../package/contents/ui/CompactRepresentation.qml
    ../package/contents/ui/FullRepresentation.qml
    ../package/contents/ui/DirectoriesPage.qml
    ../package/contents/ui/DevicesPage.qml
    ../package/contents/ui/DownloadsPage.qml
    ../package/contents/ui/TopLevelView.qml
    ../package/contents/ui/TopLevelItem.qml
    ../package/contents/ui/DetailView.qml
    ../package/contents/ui/DetailItem.qml
    ../package/contents/ui/ToolTipView.qml
    ../package/contents/ui/TinyButton.qml
    ../package/contents/ui/TinyButtonStyle.qml
    ../package/contents/ui/ButtonShadow.qml
    ../package/contents/ui/StatisticsView.qml
    ../package/contents/ui/main.qml)
list(APPEND QML_SRC_FILES ${PLASMOID_FILES})

set(TS_FILES ../translations/${META_PROJECT_NAME}_de_DE.ts ../translations/${META_PROJECT_NAME}_en_US.ts)

# find c++utilities
find_package(c++utilities 4.10.0 REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${CPP_UTILITIES_MODULE_DIRS})

# find qtutilities
find_package(qtutilities 5.13.0 REQUIRED)
use_qt_utilities()

# check whether qtutilities supports DBus notifications
get_target_property(QT_UTILITIES_COMPILE_DEFINITIONS ${QT_UTILITIES_LIB} INTERFACE_COMPILE_DEFINITIONS)
list(FIND QT_UTILITIES_COMPILE_DEFINITIONS QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS)
if (QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS LESS 0)
    message(
        FATAL_ERROR
            "The provided build of Qt Utilities does not support D-Bus notifications which is required for the Plasmoid.")
endif ()

# find backend libraries
find_package(syncthingconnector ${META_APP_VERSION} REQUIRED)
use_syncthingconnector()
find_package(syncthingmodel ${META_APP_VERSION} REQUIRED)
use_syncthingmodel()
find_package(syncthingwidgets ${META_APP_VERSION} REQUIRED)
use_syncthingwidgets()

# link also explicitely against the following Qt 5 modules
list(APPEND ADDITIONAL_QT_MODULES Network Qml)
list(APPEND ADDITIONAL_KF_MODULES Plasma)

include(BasicConfig)
include(QtGuiConfig)
include(QtConfig)
include(WindowsResources)
include(LibraryTarget)
include(ConfigHeader)

# what ever this does, it is done
kcoreaddons_desktop_to_json("${META_PROJECT_NAME}" ../package/metadata.desktop)
