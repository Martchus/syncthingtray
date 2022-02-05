import QtQml 2.3
import QtQuick 2.7
import QtQuick.Layouts 1.2
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kquickcontrolsaddons 2.0
import martchus.syncthingplasmoid 0.6 as SyncthingPlasmoid

RowLayout {
    id: toolBar
    Layout.fillWidth: true
    spacing: PlasmaCore.Units.smallSpacing
    Layout.minimumHeight: units.iconSizes.medium
    Layout.maximumHeight: units.iconSizes.medium

    readonly property bool showExtraButtons: !(plasmoid.containmentDisplayHints & PlasmaCore.Types.ContainmentDrawsPlasmoidHeading)

    ToolButton {
        id: connectButton
        states: [
            State {
                name: "disconnected"
                PropertyChanges {
                    target: connectButton
                    text: qsTr("Connect")
                    icon.source: plasmoid.nativeInterface.faUrl + "refresh"
                    visible: true
                }
            },
            State {
                name: "connecting"
                PropertyChanges {
                    target: connectButton
                    visible: false
                }
            },
            State {
                name: "paused"
                PropertyChanges {
                    target: connectButton
                    text: qsTr("Resume")
                    icon.source: plasmoid.nativeInterface.faUrl + "play"
                    visible: true
                }
            },
            State {
                name: "idle"
                PropertyChanges {
                    target: connectButton
                    text: qsTr("Pause")
                    icon.source: plasmoid.nativeInterface.faUrl + "pause"
                    visible: true
                }
            }
        ]
        state: {
            switch (plasmoid.nativeInterface.connection.status) {
            case SyncthingPlasmoid.Data.Disconnected:
                return "disconnected"
            case SyncthingPlasmoid.Data.Reconnecting:
                return "connecting";
            case SyncthingPlasmoid.Data.Paused:
                return "paused"
            default:
                return "idle"
            }
        }
        onClicked: {
            switch (plasmoid.nativeInterface.connection.status) {
            case SyncthingPlasmoid.Data.Disconnected:
                plasmoid.nativeInterface.connection.connect()
                break
            case SyncthingPlasmoid.Data.Reconnecting:
                break
            case SyncthingPlasmoid.Data.Paused:
                plasmoid.nativeInterface.connection.resumeAllDevs()
                break
            default:
                plasmoid.nativeInterface.connection.pauseAllDevs()
                break
            }
        }
        PlasmaComponents3.ToolTip {
            text: connectButton.text
        }
        Shortcut {
            sequence: "Ctrl+Shift+P"
            onActivated: connectButton.clicked()
        }
    }
    ToolButton {
        id: startStopButton
        states: [
            State {
                name: "running"
                PropertyChanges {
                    target: startStopButton
                    visible: true
                    text: qsTr("Stop")
                    icon.source: plasmoid.nativeInterface.faUrl + "stop"
                }
                PropertyChanges {
                    target: startStopToolTip
                    text: (plasmoid.nativeInterface.service.userScope ? "systemctl --user stop " : "systemctl stop ")
                             + plasmoid.nativeInterface.service.unitName
                }
            },
            State {
                name: "stopped"
                PropertyChanges {
                    target: startStopButton
                    visible: true
                    text: qsTr("Start")
                    icon.source: plasmoid.nativeInterface.faUrl + "play"
                }
                PropertyChanges {
                    target: startStopToolTip
                    text: (plasmoid.nativeInterface.service.userScope ? "systemctl --user start " : "systemctl start ")
                             + plasmoid.nativeInterface.service.unitName
                }
            },
            State {
                name: "irrelevant"
                PropertyChanges {
                    target: startStopButton
                    visible: false
                }
            }
        ]
        state: {
            var nativeInterface = plasmoid.nativeInterface
            // the systemd unit status is only relevant when connected to the local instance
            if (!nativeInterface.local
                    || !nativeInterface.startStopEnabled) {
                return "irrelevant"
            }
            // show start/stop button only when the configured unit is available
            var service = nativeInterface.service
            if (!service || !service.systemdAvailable) {
                return "irrelevant"
            }
            return service.running ? "running" : "stopped"
        }
        onClicked: plasmoid.nativeInterface.service.toggleRunning()
        PlasmaComponents3.ToolTip {
            id: startStopToolTip
        }
        Shortcut {
            sequence: "Ctrl+Shift+S"
            onActivated: {
                if (startStopButton.visible) {
                    startStopButton.clicked()
                }
            }
        }
    }
    Item {
        Layout.fillWidth: true
    }
    PlasmaComponents3.ToolButton {
        id: showNewNotifications
        icon.name: "emblem-warning"
        visible: plasmoid.nativeInterface.notificationsAvailable
        onClicked: {
            plasmoid.nativeInterface.showNotificationsDialog()
            plasmoid.expanded = false
        }
        PlasmaComponents3.ToolTip {
            text: qsTr("Show new notifications")
        }
        Shortcut {
            sequence: "Ctrl+N"
            onActivated: {
                if (showNewNotifications.visible) {
                    showNewNotifications.clicked()
                }
            }
        }
    }
    ToolButton {
        icon.source: plasmoid.nativeInterface.faUrl + "info"
        visible: showExtraButtons
        onClicked: {
            plasmoid.nativeInterface.showAboutDialog()
            plasmoid.expanded = false
        }
        PlasmaComponents3.ToolTip {
            text: qsTr("About Syncthing Tray")
        }
    }
    ToolButton {
        id: showOwnIdButton
        icon.source: plasmoid.nativeInterface.faUrl + "qrcode"
        visible: showExtraButtons
        onClicked: {
            plasmoid.nativeInterface.showOwnDeviceId()
            plasmoid.expanded = false
        }
        PlasmaComponents3.ToolTip {
            text: qsTr("Show own device ID")
        }
        Shortcut {
            sequence: "Ctrl+I"
            onActivated: showOwnIdButton.clicked()
        }
    }
    ToolButton {
        id: showLogButton
        icon.source: plasmoid.nativeInterface.faUrl + "file-text"
        visible: showExtraButtons
        onClicked: {
            plasmoid.nativeInterface.showLog()
            plasmoid.expanded = false
        }
        PlasmaComponents3.ToolTip {
            text: qsTr("Show Syncthing log")
        }
        Shortcut {
            sequence: "Ctrl+L"
            onActivated: showLogButton.clicked()
        }
    }
    ToolButton {
        id: rescanAllDirsButton
        icon.source: plasmoid.nativeInterface.faUrl + "refresh"
        onClicked: plasmoid.nativeInterface.connection.rescanAllDirs()
        PlasmaComponents3.ToolTip {
            text: qsTr("Rescan all directories")
        }
        Shortcut {
            sequence: "Ctrl+Shift+R"
            onActivated: rescanAllDirsButton.clicked()
        }
    }
    ToolButton {
        id: settingsButton
        icon.source: plasmoid.nativeInterface.faUrl + "cog"
        visible: showExtraButtons
        onClicked: {
            plasmoid.nativeInterface.showSettingsDlg()
            plasmoid.expanded = false
        }
        PlasmaComponents3.ToolTip {
            text: qsTr("Settings")
        }
        Shortcut {
            sequence: "Ctrl+S"
            onActivated: settingsButton.clicked()
        }
    }
    ToolButton {
        id: webUIButton
        icon.source: plasmoid.nativeInterface.faUrl + "syncthing"
        onClicked: {
            plasmoid.nativeInterface.showWebUI()
            plasmoid.expanded = false
        }
        PlasmaComponents3.ToolTip {
            text: qsTr("Open Syncthing")
        }
        Shortcut {
            sequence: "Ctrl+W"
            onActivated: webUIButton.clicked()
        }
    }
    PlasmaComponents3.ComboBox {
        id: connectionConfigsMenu
        model: plasmoid.nativeInterface.connectionConfigNames
        visible: plasmoid.nativeInterface.connectionConfigNames.length > 1
        currentIndex: plasmoid.nativeInterface.currentConnectionConfigIndex
        onCurrentIndexChanged: plasmoid.nativeInterface.currentConnectionConfigIndex = currentIndex
        Layout.fillWidth: true
        Layout.maximumWidth: implicitWidth
        Shortcut {
            sequence: "Ctrl+Shift+C"
            onActivated: connectionConfigsMenu.popup()
        }
    }
}
