import QtQuick 2.7
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

    // ensure keyboard events can be received after initialization
    Component.onCompleted: forceActiveFocus()

    // update the size when settings changed
    Connections {
        target: plasmoid.nativeInterface
        onSizeChanged: {
            switch (plasmoid.location) {
            case PlasmaCore.Types.Floating:
            case PlasmaCore.Types.TopEdge:
            case PlasmaCore.Types.BottomEdge:
            case PlasmaCore.Types.LeftEdge:
            case PlasmaCore.Types.RightEdge:
                // set the parent's width and height so it will shrink again when decreasing the size
                var size = plasmoid.nativeInterface.size
                parent.width = units.gridUnit * size.width
                parent.height = units.gridUnit * size.height
                break
            default:
                ;
            }
        }
    }

    // define shortcuts to trigger actions for currently selected item
    function clickCurrentItemButton(buttonName) {
        mainTabGroup.currentTab.item.view.clickCurrentItemButton(buttonName)
    }
    Shortcut {
        sequence: "Ctrl+R"
        onActivated: clickCurrentItemButton("rescanButton")
    }
    Shortcut {
        sequence: "Ctrl+P"
        onActivated: clickCurrentItemButton("resumePauseButton")
    }
    Shortcut {
        sequence: "Ctrl+O"
        onActivated: clickCurrentItemButton("openButton")
    }

    // define custom key handling for switching tabs, selecting items and filtering
    Keys.onPressed: {
        // note: event only received after clicking the tab buttons in plasmoidviewer
        // but works as expected in plasmashell
        switch (event.key) {
        case Qt.Key_Up:
            switch (event.modifiers) {
            case Qt.NoModifier:
                // select previous item in current tab
                mainTabGroup.currentTab.item.view.decrementCurrentIndex()
                break
            case Qt.ShiftModifier:
                // select previous connection
                --plasmoid.nativeInterface.currentConnectionConfigIndex
                break
            }
            break
        case Qt.Key_Down:
            switch (event.modifiers) {
            case Qt.NoModifier:
                // select next item in current tab
                mainTabGroup.currentTab.item.view.incrementCurrentIndex()
                break
            case Qt.ShiftModifier:
                // select previous connection
                ++plasmoid.nativeInterface.currentConnectionConfigIndex
                break
            }
            break
        case Qt.Key_Left:
            // select previous tab
            switch (mainTabGroup.currentTab) {
            case dirsPage:
                recentChangesPage.clicked()
                break
            case devicesPage:
                dirsTabButton.clicked()
                break
            case downloadsPage:
                devsTabButton.clicked()
                break
            case recentChangesPage:
                downloadsTabButton.clicked()
                break
            }
            break
        case Qt.Key_Right:
            // select next tab
            switch (mainTabGroup.currentTab) {
            case dirsPage:
                devsTabButton.clicked()
                break
            case devicesPage:
                downloadsTabButton.clicked()
                break
            case downloadsPage:
                recentChangesTabButton.clicked()
                break
            case recentChangesPage:
                dirsTabButton.clicked()
            }
            break
        case Qt.Key_Enter:

            // fallthrough
        case Qt.Key_Return:
            // toggle expanded state of current item
            var currentItem = mainTabGroup.currentTab.item.view.currentItem
            if (currentItem) {
                currentItem.expanded = !currentItem.expanded
            }
            break
        case Qt.Key_Escape:
            var filter = findCurrentFilter()
            if (filter && filter.text !== "") {
                // reset filter
                filter.explicitelyShown = false
                filter.text = ""
                event.accepted = true
            } else {
                // hide plasmoid
                plasmoid.expanded = false
            }
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
        case Qt.Key_4:
            recentChangesTabButton.clicked()
            break
        default:
            sendKeyEventToFilter(event)
            return
        }
        event.accepted = true
    }

    function findCurrentFilter() {
        return mainTabGroup.currentTab.item.filter
    }

    function sendKeyEventToFilter(event) {
        var filter = findCurrentFilter()
        if (!filter || event.text === "" || filter.activeFocus) {
            return
        }
        if (event.key === Qt.Key_Backspace && filter.text === "") {
            filter.explicitelyShown = false
            return
        }
        if (event.matches(StandardKey.Paste)) {
            filter.paste()
        } else {
            filter.text = ""
            filter.text += event.text
        }
        filter.forceActiveFocus()
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
                        icon: "view-refresh"
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
                        icon: "media-playback-start"
                        visible: true
                    }
                },
                State {
                    name: "idle"
                    PropertyChanges {
                        target: connectButton
                        text: qsTr("Pause")
                        icon: "media-playback-pause"
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
            tooltip: text
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
            Shortcut {
                sequence: "Ctrl+Shift+P"
                onActivated: connectButton.clicked()
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
                        tooltip: (plasmoid.nativeInterface.service.userScope ? "systemctl --user stop " : "systemctl stop ")
                                 + plasmoid.nativeInterface.service.unitName
                        icon: "process-stop"
                    }
                },
                State {
                    name: "stopped"
                    PropertyChanges {
                        target: startStopButton
                        visible: true
                        text: qsTr("Start")
                        tooltip: (plasmoid.nativeInterface.service.userScope ? "systemctl --user start " : "systemctl start ")
                                 + plasmoid.nativeInterface.service.unitName
                        icon: "system-run"
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
            style: TinyButtonStyle {}
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
        TinyButton {
            id: showNewNotifications
            tooltip: qsTr("Show new notifications")
            iconSource: "emblem-warning"
            visible: plasmoid.nativeInterface.notificationsAvailable
            onClicked: {
                plasmoid.nativeInterface.showNotificationsDialog()
                plasmoid.expanded = false
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
        TinyButton {
            tooltip: qsTr("About Syncthing Tray")
            icon: "help-about"
            onClicked: {
                plasmoid.nativeInterface.showAboutDialog()
                plasmoid.expanded = false
            }
        }
        TinyButton {
            id: showOwnIdButton
            tooltip: qsTr("Show own device ID")
            icon: "view-barcode"
            onClicked: {
                plasmoid.nativeInterface.showOwnDeviceId()
                plasmoid.expanded = false
            }
            Shortcut {
                sequence: "Ctrl+I"
                onActivated: showOwnIdButton.clicked()
            }
        }
        TinyButton {
            id: showLogButton
            tooltip: qsTr("Show Syncthing log")
            icon: "text-x-generic"
            onClicked: {
                plasmoid.nativeInterface.showLog()
                plasmoid.expanded = false
            }
            Shortcut {
                sequence: "Ctrl+L"
                onActivated: showLogButton.clicked()
            }
        }
        TinyButton {
            id: rescanAllDirsButton
            tooltip: qsTr("Rescan all directories")
            icon: "folder-sync"
            onClicked: plasmoid.nativeInterface.connection.rescanAllDirs()
            Shortcut {
                sequence: "Ctrl+Shift+R"
                onActivated: rescanAllDirsButton.clicked()
            }
        }
        TinyButton {
            id: settingsButton
            tooltip: qsTr("Settings")
            icon: "preferences-other"
            onClicked: {
                plasmoid.nativeInterface.showSettingsDlg()
                plasmoid.expanded = false
            }
            Shortcut {
                sequence: "Ctrl+S"
                onActivated: settingsButton.clicked()
            }
        }
        TinyButton {
            id: webUIButton
            tooltip: qsTr("Open Syncthing")
            icon: plasmoid.nativeInterface.syncthingIcon
            onClicked: {
                plasmoid.nativeInterface.showWebUI()
                plasmoid.expanded = false
            }
            Shortcut {
                sequence: "Ctrl+W"
                onActivated: webUIButton.clicked()
            }
        }
        TinyButton {
            id: connectionsButton
            text: plasmoid.nativeInterface.currentConnectionConfigName
            icon: "network-connect"
            paddingEnabled: true
            enforceMenuArrow: true
            onClicked: connectionConfigsMenu.toggle(x, y + height)
            visible: plasmoid.nativeInterface.connectionConfigNames.length > 1
            Shortcut {
                sequence: "Ctrl+Shift+C"
                onActivated: connectionsButton.clicked()
            }
        }
        PlasmaComponents.Menu {
            id: connectionConfigsMenu
            function toggle(x, y) {
                if (connectionConfigsMenu.status === PlasmaComponents.DialogStatus.Open) {
                    close()
                    return
                }
                var nativeInterface = plasmoid.nativeInterface
                var configNames = nativeInterface.connectionConfigNames
                var currentIndex = nativeInterface.currentConnectionConfigIndex
                clearMenuItems()
                for (var i = 0, count = configNames.length; i !== count; ++i) {
                    addMenuItem(menuItem.createObject(connectButton, {
                                                          "text": configNames[i],
                                                          "checked": i === currentIndex,
                                                          "index": i
                                                      }))
                }
                open(x, y)
            }
        }
        Component {
            id: menuItem
            PlasmaComponents.MenuItem {
                property int index: -1
                checkable: true
                onClicked: {
                    plasmoid.nativeInterface.currentConnectionConfigIndex = index
                    connectionConfigsMenu.close()
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

    // global statistics and traffic
    GridLayout {
        Layout.leftMargin: 5
        Layout.fillWidth: true
        Layout.fillHeight: false
        columns: 3
        rowSpacing: 1
        columnSpacing: 4

        PlasmaCore.IconItem {
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            source: "globe"
        }
        StatisticsView {
            Layout.leftMargin: 4
            statistics: plasmoid.nativeInterface.globalStatistics
            context: qsTr("Global")
        }

        IconLabel {
            Layout.leftMargin: 10
            iconSource: plasmoid.nativeInterface.loadFontAwesomeIcon(
                            "cloud-download-alt")
            iconOpacity: plasmoid.nativeInterface.hasIncomingTraffic ? 1.0 : 0.5
            text: plasmoid.nativeInterface.incomingTraffic
            tooltip: qsTr("Global incoming traffic")
        }

        PlasmaCore.IconItem {
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            source: "user-home-symbolic"
        }
        StatisticsView {
            Layout.leftMargin: 4
            statistics: plasmoid.nativeInterface.localStatistics
            context: qsTr("Local")
        }

        IconLabel {
            Layout.leftMargin: 10
            iconSource: plasmoid.nativeInterface.loadFontAwesomeIcon(
                            "cloud-upload-alt")
            iconOpacity: plasmoid.nativeInterface.hasOutgoingTraffic ? 1.0 : 0.5
            text: plasmoid.nativeInterface.outgoingTraffic
            tooltip: qsTr("Global outgoing traffic")
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
        id: tabWidget
        spacing: 0
        Layout.minimumWidth: units.gridUnit * plasmoid.nativeInterface.size.width
        Layout.minimumHeight: units.gridUnit * plasmoid.nativeInterface.size.height

        ColumnLayout {
            spacing: 0
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft

            PlasmaComponents.TabBar {
                id: tabBar
                tabPosition: Qt.LeftEdge
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft

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
                PlasmaComponents.TabButton {
                    id: recentChangesTabButton
                    //text: qsTr("Recent changes")
                    iconSource: "document-open-recent-symbolic"
                    tab: recentChangesPage
                }
            }
            Item {
                Layout.fillHeight: true
            }
            TinyButton {
                id: searchButton
                icon: "search"
                enabled: mainTabGroup.currentTab === dirsPage
                tooltip: qsTr("Toggle filter")
                onClicked: {
                    var filter = findCurrentFilter()
                    if (!filter) {
                        return
                    }
                    if (!filter.explicitelyShown) {
                        filter.explicitelyShown = true
                        filter.forceActiveFocus()
                    } else {
                        filter.explicitelyShown = false
                        filter.text = ""
                    }
                }
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
            PlasmaExtras.ConditionalLoader {
                id: recentChangesPage
                when: mainTabGroup.currentTab === recentChangesPage
                source: Qt.resolvedUrl("RecentChangesPage.qml")
            }
        }
    }
}
