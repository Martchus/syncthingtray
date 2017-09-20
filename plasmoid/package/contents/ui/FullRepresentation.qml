import QtQuick 2.3
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQml 2.2
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.components 2.0 as PlasmaComponents
import martchus.syncthingplasmoid 0.6 as SyncthingPlasmoid

ColumnLayout {
    id: root

    // define minimum size
    Layout.minimumWidth: units.gridUnit * 20
    Layout.minimumHeight: units.gridUnit * 15

    // define function to update the size according to the settings
    // when "floating" (shown as popup)
    function updateSize() {
        switch (plasmoid.location) {
        case PlasmaCore.Types.Floating:

        case PlasmaCore.Types.TopEdge:

        case PlasmaCore.Types.BottomEdge:

        case PlasmaCore.Types.LeftEdge:

        case PlasmaCore.Types.RightEdge:
            var size = plasmoid.nativeInterface.size
            parent.width = units.gridUnit * size.width
            parent.height = units.gridUnit * size.height
            break
        default:
            ;
        }
    }

    // update the size when becoming visible
    onVisibleChanged: {
        if (visible) {
            updateSize()
        }
    }

    // update the size when settings changed
    Connections {
        target: plasmoid.nativeInterface
        onSizeChanged: updateSize()
    }

    // shortcut handling
    Keys.onPressed: {
        // FIXME: currently only works after clicking the tab buttons
        // TODO: add more shortcuts
        switch (event.key) {
        case Qt.Key_Up:
            mainTabGroup.currentTab.item.view.decrementCurrentIndex()
            event.accepted = true
            break
        case Qt.Key_Down:
            mainTabGroup.currentTab.item.view.incrementCurrentIndex()
            event.accepted = true
            break
        case Qt.Key_Left:
            switch (mainTabGroup.currentTab) {
            case dirsPage:
                downloadsTabButton.clicked()
                break
            case devicesPage:
                dirsTabButton.clicked()
                break
            case downloadsPage:
                devsTabButton.clicked()
                break
            }
            break
        case Qt.Key_Right:
            switch (mainTabGroup.currentTab) {
            case dirsPage:
                devsTabButton.clicked()
                break
            case devicesPage:
                downloadsTabButton.clicked()
                break
            case downloadsPage:
                dirsTabButton.clicked()
                break
            }
            break
        case Qt.Key_Enter:

        case Qt.Key_Return:
            var currentItem = mainTabGroup.currentTab.item.view.currentItem
            if (currentItem) {
                currentItem.expanded = !currentItem.expanded
            }
            break
        case Qt.Key_Escape:
            break
        case Qt.Key_1:
            dirsTabButton.clicked()
            break
        case Qt.Key_2:
            devsTabButton.clicked()
            break
        case Qt.Key_3:
            downloadsTabButton.clicked()
            break
        default:
            break
        }
    }

    // heading and right-corner buttons
    RowLayout {
        id: toolBar
        Layout.fillWidth: true

        TinyButton {
            id: connectButton

            states: [
                State {
                    name: "disconnected"
                    PropertyChanges {
                        target: connectButton
                        text: qsTr("Connect")
                        iconSource: "view-refresh"
                    }
                },
                State {
                    name: "paused"
                    PropertyChanges {
                        target: connectButton
                        text: qsTr("Resume")
                        iconSource: "media-playback-start"
                    }
                },
                State {
                    name: "idle"
                    PropertyChanges {
                        target: connectButton
                        text: qsTr("Pause")
                        iconSource: "media-playback-pause"
                    }
                }
            ]
            state: {
                switch (plasmoid.nativeInterface.connection.status) {
                case SyncthingPlasmoid.Data.Disconnected:

                case SyncthingPlasmoid.Data.Reconnecting:
                    return "disconnected"
                case SyncthingPlasmoid.Data.Paused:
                    return "paused"
                default:
                    return "idle"
                }
            }
            tooltip: text
            onClicked: {
                switch (plasmoid.nativeInterface.connection.status) {
                case SyncthingPlasmoid.Data.Disconnected:

                case SyncthingPlasmoid.Data.Reconnecting:
                    plasmoid.nativeInterface.connection.connect()
                    break
                case SyncthingPlasmoid.Data.Paused:
                    plasmoid.nativeInterface.connection.resumeAllDevs()
                    break
                default:
                    plasmoid.nativeInterface.connection.pauseAllDevs()
                    break
                }
            }
        }
        TinyButton {
            id: startStopButton

            states: [
                State {
                    name: "running"
                    PropertyChanges {
                        target: startStopButton
                        visible: true
                        text: qsTr("Stop")
                        tooltip: "systemctl --user stop "
                                 + plasmoid.nativeInterface.service.unitName
                        iconSource: "process-stop"
                    }
                },
                State {
                    name: "stopped"
                    PropertyChanges {
                        target: startStopButton
                        visible: true
                        text: qsTr("Start")
                        tooltip: "systemctl --user start "
                                 + plasmoid.nativeInterface.service.unitName
                        iconSource: "system-run"
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
                        || !nativeInterface.startStopForServiceEnabled) {
                    return "irrelevant"
                }
                // show start/stop button only when the configured unit is available
                var service = nativeInterface.service
                if (!service || !service.unitAvailable) {
                    return "irrelevant"
                }
                return service.running ? "running" : "stopped"
            }
            onClicked: plasmoid.nativeInterface.service.toggleRunning()
            style: TinyButtonStyle {
            }
        }
        Item {
            Layout.fillWidth: true
        }
        TinyButton {
            tooltip: qsTr("Show own device ID")
            iconSource: "view-barcode"
            onClicked: {
                plasmoid.nativeInterface.showOwnDeviceId()
                plasmoid.expanded = false
            }
        }
        TinyButton {
            tooltip: qsTr("Show Syncthing log")
            iconSource: "text-x-generic"
            onClicked: {
                plasmoid.nativeInterface.showLog()
                plasmoid.expanded = false
            }
        }
        TinyButton {
            tooltip: qsTr("Rescan all directories")
            iconSource: "view-refresh"
            onClicked: plasmoid.nativeInterface.connection.rescanAllDirs()
        }
        TinyButton {
            tooltip: qsTr("Settings")
            iconSource: "preferences-other"
            onClicked: {
                plasmoid.nativeInterface.showSettingsDlg()
                plasmoid.expanded = false
            }
        }
        TinyButton {
            tooltip: qsTr("Web UI")
            iconSource: "internet-web-browser"
            onClicked: {
                plasmoid.nativeInterface.showWebUI()
                plasmoid.expanded = false
            }
        }
    }

    PlasmaCore.SvgItem {
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 2
        elementId: "horizontal-line"
        svg: PlasmaCore.Svg {
            imagePath: "widgets/line"
        }
    }

    // traffic and connection selection
    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: false

        PlasmaCore.IconItem {
            source: "network-card"
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
        }
        ColumnLayout {
            Layout.fillHeight: true
            spacing: 1

            PlasmaComponents.Label {
                text: qsTr("In")
            }
            PlasmaComponents.Label {
                text: qsTr("Out")
            }
        }
        ColumnLayout {
            Layout.fillHeight: true
            spacing: 1

            PlasmaComponents.Label {
                text: plasmoid.nativeInterface.incomingTraffic
            }
            PlasmaComponents.Label {
                text: plasmoid.nativeInterface.outgoingTraffic
            }
        }
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        TinyButton {
            text: plasmoid.nativeInterface.currentConnectionConfigName
            iconSource: "network-connect"
            paddingEnabled: true
            // FIXME: figure out why menu doesn't work in plasmoidviewer using NVIDIA driver
            // (works with plasmawindowed or Intel graphics)
            menu: Menu {
                id: connectionConfigsMenu

                ExclusiveGroup {
                    id: connectionConfigsExclusiveGroup
                }

                Instantiator {
                    model: plasmoid.nativeInterface.connectionConfigNames

                    MenuItem {
                        text: model.modelData
                        checkable: true
                        checked: plasmoid.nativeInterface.currentConnectionConfigIndex === index
                        exclusiveGroup: connectionConfigsExclusiveGroup
                        onTriggered: {
                            plasmoid.nativeInterface.currentConnectionConfigIndex = index
                        }
                    }
                    onObjectAdded: connectionConfigsMenu.insertItem(index,
                                                                    object)
                    onObjectRemoved: connectionConfigsMenu.removeItem(object)
                }
            }
        }
    }

    PlasmaCore.SvgItem {
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 2
        elementId: "horizontal-line"
        svg: PlasmaCore.Svg {
            imagePath: "widgets/line"
        }
    }

    // tab "widget"
    RowLayout {
        spacing: 0

        PlasmaComponents.TabBar {
            id: tabBar
            tabPosition: Qt.LeftEdge
            anchors.top: parent.top

            PlasmaComponents.TabButton {
                id: dirsTabButton
                //text: qsTr("Directories")
                iconSource: "folder-symbolic"
                tab: dirsPage
            }
            PlasmaComponents.TabButton {
                id: devsTabButton
                //text: qsTr("Devices")
                iconSource: "network-server-symbolic"
                tab: devicesPage
            }
            PlasmaComponents.TabButton {
                id: downloadsTabButton
                //text: qsTr("Downloads")
                iconSource: "folder-download-symbolic"
                tab: downloadsPage
            }
        }
        PlasmaCore.SvgItem {
            Layout.preferredWidth: 2
            Layout.fillHeight: true
            elementId: "vertical-line"
            svg: PlasmaCore.Svg {
                imagePath: "widgets/line"
            }
        }
        PlasmaComponents.TabGroup {
            id: mainTabGroup
            currentTab: dirsPage
            Layout.fillWidth: true
            Layout.fillHeight: true

            PlasmaExtras.ConditionalLoader {
                id: dirsPage
                when: mainTabGroup.currentTab === dirsPage
                source: Qt.resolvedUrl("DirectoriesPage.qml")
            }
            PlasmaExtras.ConditionalLoader {
                id: devicesPage
                when: mainTabGroup.currentTab === devicesPage
                source: Qt.resolvedUrl("DevicesPage.qml")
            }
            PlasmaExtras.ConditionalLoader {
                id: downloadsPage
                when: mainTabGroup.currentTab === downloadsPage
                source: Qt.resolvedUrl("DownloadsPage.qml")
            }
        }
    }
}
