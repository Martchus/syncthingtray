import QtQml 2.3
import QtQuick 2.7
import QtQuick.Layouts 1.2
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kirigami 2.20 as Kirigami
import org.kde.kquickcontrolsaddons 2.0
import martchus.syncthingplasmoid 0.6 as SyncthingPlasmoid

RowLayout {
    id: toolBar
    Layout.fillWidth: true
    spacing: Kirigami.Units.smallSpacing
    Layout.minimumHeight: Kirigami.Units.iconSizes.medium
    Layout.maximumHeight: Kirigami.Units.iconSizes.medium

    readonly property bool showExtraButtons: !(plasmoid.containmentDisplayHints & PlasmaCore.Types.ContainmentDrawsPlasmoidHeading)

    ToolButton {
        id: connectButton
        states: [
            State {
                name: "disconnected"
                PropertyChanges {
                    target: connectButton
                    text: qsTr("Connect")
                    icon.source: plasmoid.faUrl + "refresh"
                    enabled: true
                }
            },
            State {
                name: "connecting"
                PropertyChanges {
                    target: connectButton
                    text: qsTr("Connecting …")
                    icon.source: plasmoid.faUrl + "refresh"
                    enabled: false
                }
            },
            State {
                name: "paused"
                PropertyChanges {
                    target: connectButton
                    text: qsTr("Resume")
                    icon.source: plasmoid.faUrl + "play"
                    enabled: true
                }
            },
            State {
                name: "idle"
                PropertyChanges {
                    target: connectButton
                    text: qsTr("Pause")
                    icon.source: plasmoid.faUrl + "pause"
                    enabled: true
                }
            }
        ]
        state: {
            switch (plasmoid.connection.status) {
            case SyncthingPlasmoid.Data.Disconnected:
                return plasmoid.connection.connecting ? "connecting" : "disconnected"
            case SyncthingPlasmoid.Data.Reconnecting:
                return "connecting";
            case SyncthingPlasmoid.Data.Paused:
                return "paused"
            default:
                return "idle"
            }
        }
        onClicked: {
            switch (plasmoid.connection.status) {
            case SyncthingPlasmoid.Data.Disconnected:
                plasmoid.connection.connect()
                break
            case SyncthingPlasmoid.Data.Reconnecting:
                break
            case SyncthingPlasmoid.Data.Paused:
                plasmoid.connection.resumeAllDevs()
                break
            default:
                plasmoid.connection.pauseAllDevs()
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
                    icon.source: plasmoid.faUrl + "stop"
                }
                PropertyChanges {
                    target: startStopToolTip
                    text: (plasmoid.service.userScope ? "systemctl --user stop " : "systemctl stop ")
                             + plasmoid.service.unitName
                }
            },
            State {
                name: "stopped"
                PropertyChanges {
                    target: startStopButton
                    visible: true
                    text: qsTr("Start")
                    icon.source: plasmoid.faUrl + "play"
                }
                PropertyChanges {
                    target: startStopToolTip
                    text: (plasmoid.service.userScope ? "systemctl --user start " : "systemctl start ")
                             + plasmoid.service.unitName
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
            // the systemd unit status is only relevant when connected to the local instance
            if (!plasmoid.local || !plasmoid.startStopEnabled) {
                return "irrelevant"
            }
            // show start/stop button only when the configured unit is available
            var service = plasmoid.service
            if (!service || !service.systemdAvailable) {
                return "irrelevant"
            }
            return service.running ? "running" : "stopped"
        }
        onClicked: plasmoid.service.toggleRunning()
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
        visible: plasmoid.notificationsAvailable
        onClicked: {
            plasmoid.showNotificationsDialog()
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
        icon.source: plasmoid.faUrl + "info"
        visible: showExtraButtons
        onClicked: {
            plasmoid.showAboutDialog()
            plasmoid.expanded = false
        }
        PlasmaComponents3.ToolTip {
            text: qsTr("About Syncthing Tray")
        }
    }
    ToolButton {
        id: showOwnIdButton
        icon.source: plasmoid.faUrl + "qrcode"
        visible: showExtraButtons
        onClicked: {
            plasmoid.showOwnDeviceId()
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
        icon.source: plasmoid.faUrl + "file-text"
        visible: showExtraButtons
        onClicked: {
            plasmoid.showLog()
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
        icon.source: plasmoid.faUrl + "refresh"
        onClicked: plasmoid.connection.rescanAllDirs()
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
        icon.source: plasmoid.faUrl + "cog"
        visible: showExtraButtons
        onClicked: {
            plasmoid.showSettingsDlg()
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
        icon.source: plasmoid.faUrl + "syncthing"
        onClicked: {
            plasmoid.showWebUI()
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
        model: plasmoid.connectionConfigNames
        visible: plasmoid.connectionConfigNames.length > 1
        currentIndex: plasmoid.currentConnectionConfigIndex
        onCurrentIndexChanged: plasmoid.currentConnectionConfigIndex = currentIndex
        Layout.fillWidth: true
        Layout.maximumWidth: implicitWidth
        Shortcut {
            sequence: "Ctrl+Shift+C"
            onActivated: connectionConfigsMenu.popup()
        }
    }
}
