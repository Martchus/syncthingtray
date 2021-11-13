import QtQuick 2.8
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.15 as QQ2 // for ComboBox (PlasmaComponents3 version clips the end of the text)
import QtQml 2.2
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.components 2.0 as PlasmaComponents // for vertical TabBar
import org.kde.plasma.components 3.0 as PlasmaComponents3
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

        ToolButton {
            id: connectButton
            states: [
                State {
                    name: "disconnected"
                    PropertyChanges {
                        target: connectButton
                        text: qsTr("Connect")
                        icon.source: "image://fa/refresh"
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
                        icon.source: "image://fa/play"
                        visible: true
                    }
                },
                State {
                    name: "idle"
                    PropertyChanges {
                        target: connectButton
                        text: qsTr("Pause")
                        icon.source: "image://fa/pause"
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
            PlasmaComponents3.ToolTip {
                text: connectButton.text
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
                        icon.source: "image://fa/stop"
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
                        icon.source: "image://fa/play"
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
            icon.source: "image://fa/info"
            PlasmaComponents3.ToolTip {
                text: qsTr("About Syncthing Tray")
            }
            onClicked: {
                plasmoid.nativeInterface.showAboutDialog()
                plasmoid.expanded = false
            }
        }
        ToolButton {
            id: showOwnIdButton
            icon.source: "image://fa/qrcode"
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
            icon.source: "image://fa/file-text"
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
            icon.source: "image://fa/refresh"
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
            icon.source: "image://fa/cog"
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
            icon.source: "image://fa/syncthing"
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
        QQ2.ComboBox {
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

        Image {
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            height: 16
            fillMode: Image.PreserveAspectFit
            source: "image://fa/globe"
        }
        StatisticsView {
            Layout.leftMargin: 4
            statistics: plasmoid.nativeInterface.globalStatistics
            context: qsTr("Global")
        }

        IconLabel {
            Layout.leftMargin: 10
            iconSource: "image://fa/cloud-download"
            iconOpacity: plasmoid.nativeInterface.hasIncomingTraffic ? 1.0 : 0.5
            text: plasmoid.nativeInterface.incomingTraffic
            tooltip: qsTr("Global incoming traffic")
        }

        Image {
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            height: 16
            fillMode: Image.PreserveAspectFit
            source: "image://fa/home"
        }
        StatisticsView {
            Layout.leftMargin: 4
            statistics: plasmoid.nativeInterface.localStatistics
            context: qsTr("Local")
        }

        IconLabel {
            Layout.leftMargin: 10
            iconSource: "image://fa/cloud-upload"
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
                    iconSource: plasmoid.nativeInterface.loadForkAwesomeIcon("folder")
                    tab: dirsPage
                }
                PlasmaComponents.TabButton {
                    id: devsTabButton
                    iconSource: plasmoid.nativeInterface.loadForkAwesomeIcon("sitemap")
                    tab: devicesPage
                }
                PlasmaComponents.TabButton {
                    id: downloadsTabButton
                    iconSource: plasmoid.nativeInterface.loadForkAwesomeIcon("download")
                    tab: downloadsPage
                }
                PlasmaComponents.TabButton {
                    id: recentChangesTabButton
                    iconSource: plasmoid.nativeInterface.loadForkAwesomeIcon("history")
                    tab: recentChangesPage
                }
            }
            Item {
                Layout.fillHeight: true
            }
            TinyButton {
                id: searchButton
                icon.name: "search"
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
