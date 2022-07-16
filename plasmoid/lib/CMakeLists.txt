# source files
set(HEADER_FILES syncthingapplet.h settingsdialog.h)
set(SRC_FILES syncthingapplet.cpp settingsdialog.cpp)
set(WIDGETS_UI_FILES appearanceoptionpage.ui)
foreach (PLASMOID_FILE ${PLASMOID_FILES})
    list(APPEND QML_SRC_FILES "../${PLASMOID_FILE}")
endforeach ()
set(TS_FILES ../translations/${META_PROJECT_NAME}_de_DE.ts ../translations/${META_PROJECT_NAME}_en_US.ts)

# find qtutilities
find_package(${PACKAGE_NAMESPACE_PREFIX}qtutilities${CONFIGURATION_PACKAGE_SUFFIX_QTUTILITIES} 6.7.0 REQUIRED)
use_qt_utilities()

# find qtforkawesomequickimageprovider
find_package(${PACKAGE_NAMESPACE_PREFIX}qtquickforkawesome${CONFIGURATION_PACKAGE_SUFFIX_QTFORKAWESOME} 0.0.3 REQUIRED)
use_qt_quick_fork_awesome()

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

# link also explicitly against the following Qt modules
list(APPEND ADDITIONAL_QT_MODULES Network Qml)
list(APPEND ADDITIONAL_KF_MODULES Plasma)

include(BasicConfig)
include(QtGuiConfig)
include(QtConfig)
include(WindowsResources)
include(LibraryTarget)
include(ConfigHeader)

# export some variable for further use in the overall plasmoid build script
set(META_TARGET_NAME
    "${META_TARGET_NAME}"
    PARENT_SCOPE)
set(META_ID
    "martchus.${META_TARGET_NAME}"
    PARENT_SCOPE)
set(META_PROJECT_LICENSE
    "${META_PROJECT_LICENSE}"
    PARENT_SCOPE)

# what ever this does, it is done
kcoreaddons_desktop_to_json("${META_TARGET_NAME}" "${PLASMOID_CONFIG_TARGET_FILE}" DESKTOP_TO_JSON_OUTPUT_DIR
                            "${PLASMOID_PACKAGE_DIR}")
