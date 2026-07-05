import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main
import Tray

ApplicationWindow {
    id: appWindow
    visible: true
    width: 1200
    height: 700
    title: meta.title + " - WORK IN PROGRESS"
    font: theming.font
    flags: QuickUI.extendedClientArea ? (Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint) : (Qt.Window)
    leftPadding: 0
    rightPadding: 0
    onVisibleChanged: TrayWidget.handleMainWindowVisibleChanged(appWindow.visible)
    Material.theme: theming.Material.theme
    Material.primary: theming.Material.primary
    Material.accent: theming.Material.accent
    header: Pane {
        padding: 5
        RowLayout {
            anchors.fill: parent
            Icon {
                Layout.leftMargin: 10
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: size
                Layout.preferredHeight: size
                source: SyncthingData.statusInfo.statusIcon
                width: size
                height: size
                readonly property int size: 32
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                spacing: 0
                Label {
                    Layout.fillWidth: true
                    font.weight: Font.Medium
                    text: SyncthingData.statusInfo.statusText
                    wrapMode: Text.WordWrap
                }
                Label {
                    Layout.fillWidth: true
                    font.weight: Font.Light
                    text: SyncthingData.statusInfo.additionalStatusText
                    wrapMode: Text.WordWrap
                }
            }
            MenuBar {
                id: menuBar
                Layout.preferredWidth: menuBar.contentWidth
                Layout.leftMargin: menuBar.leftPadding
                Layout.rightMargin: menuBar.rightPadding
                Menu {
                    title: qsTr("&Help")
                    icon.source: QuickUI.faUrlBase + "question-circle"
                    icon.width: QuickUI.iconSize
                    icon.height: QuickUI.iconSize
                    Label {
                        padding: 5
                        text: qsTr("Syncthing")
                        font.weight: Font.Medium
                    }
                    Action {
                        text: qsTr("Home page")
                        onTriggered: SyncthingModels.openUrlExternally("https://syncthing.net")
                    }
                    Action {
                        text: qsTr("Documentation")
                        onTriggered: SyncthingModels.openUrlExternally("https://docs.syncthing.net")
                    }
                    Action {
                        text: qsTr("Support")
                        onTriggered: SyncthingModels.openUrlExternally("https://forum.syncthing.net")
                    }
                    Action {
                        text: qsTr("Changelog")
                        onTriggered: SyncthingModels.openUrlExternally("https://github.com/syncthing/syncthing/releases")
                    }
                    Action {
                        text: qsTr("Statistics")
                        onTriggered: SyncthingModels.openUrlExternally("https://data.syncthing.net")
                    }
                    Action {
                        text: qsTr("Bugs")
                        onTriggered: SyncthingModels.openUrlExternally("https://github.com/syncthing/syncthing/issues")
                    }
                    Action {
                        text: qsTr("Source code")
                        onTriggered: SyncthingModels.openUrlExternally("https://github.com/syncthing/syncthing")
                    }
                    MenuSeparator {
                    }
                    Label {
                        padding: 5
                        text: qsTr("Syncthing Tray")
                        font.weight: Font.Medium
                    }
                    Action {
                        text: qsTr("Documentation")
                        onTriggered: SyncthingModels.openUrlExternally(SyncthingData.documentationUrl)
                    }
                    Action {
                        text: qsTr("Source code")
                        onTriggered: SyncthingModels.openUrlExternally(SyncthingData.readmeUrl.replace(/\/blob\/.*/, ""))
                    }
                    Action {
                        text: qsTr("About")
                        onTriggered: TrayWidget.showAboutDialog()
                    }
                }
                Menu {
                    title: qsTr("&Actions")
                    icon.source: QuickUI.faUrlBase + "cog"
                    icon.width: QuickUI.iconSize
                    icon.height: QuickUI.iconSize
                    Label {
                        padding: 5
                        text: qsTr("Syncthing")
                        font.weight: Font.Medium
                    }
                    Action {
                        text: qsTr("&Settings")
                        onTriggered: QuickUI.showSettings()
                        shortcut: "Ctrl+S"
                    }
                    Action {
                        text: qsTr("&Web-based UI")
                        onTriggered: TrayWidget.showSyncthingUI(true)
                        shortcut: "Ctrl+W"
                    }
                    Action {
                        text: qsTr("&Recent changes")
                        onTriggered: QuickUI.showRecentChanges()
                        shortcut: "Ctrl+R"
                    }
                    Action {
                        text: qsTr("Show &ID")
                        onTriggered: TrayWidget.showOwnDeviceId()
                        shortcut: "Ctrl+I"
                    }
                    Action {
                        text: qsTr("&Logs")
                        onTriggered: TrayWidget.showLog()
                        shortcut: "Ctrl+L"
                    }
                    Action {
                        text: qsTr("&Support Bundle")
                        enabled: false
                    }
                    Action {
                        text: qsTr("Restart")
                        onTriggered: SyncthingData.connection.restart()
                    }
                    Action {
                        text: qsTr("Shutdown")
                        onTriggered: SyncthingData.connection.shutdown()
                    }
                    MenuSeparator {
                    }
                    Label {
                        padding: 5
                        text: qsTr("Syncthing Tray")
                        font.weight: Font.Medium
                    }
                    Action {
                        text: qsTr("&Settings")
                        onTriggered: TrayWidget.showSettingsDialog()
                        shortcut: "Ctrl+Shift+S"
                    }
                    Action {
                        text: qsTr("&Wizard")
                        onTriggered: TrayWidget.showWizard()
                        shortcut: "Ctrl+Shift+W"
                    }
                }
            }
        }
    }

    Pane {
        id: mainPane
        anchors.fill: parent
        anchors.leftMargin: parent.SafeArea.margins.left
        anchors.rightMargin: parent.SafeArea.margins.right
        focus: true
        padding: 0
        topPadding: 10
        background: Rectangle {
            color: appWindow.palette.base
        }
        Keys.onEscapePressed: appWindow.close()
        ColumnLayout {
            anchors.fill: parent
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                uniformCellSizes: true
                ColumnLayout {
                    id: leftColumn
                    visible: !mainPane.compact || tabBar.currentIndex === folderButton.tabIndex
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            leftPadding: 10
                            text: qsTr("Folders")
                            font.weight: Font.Medium
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        IconOnlyButton {
                            Layout.alignment: Qt.AlignVCenter
                            text: qsTr("Add folder")
                            icon.source: QuickUI.faUrlBase + "plus"
                            onClicked: QuickUI.editDir("", "", null)
                            flat: true
                        }
                        IconOnlyButton {
                            Layout.alignment: Qt.AlignVCenter
                            text: qsTr("Pause all")
                            icon.source: QuickUI.faUrlBase + "pause"
                            onClicked: SyncthingData.connection.pauseAllDirs()
                            flat: true
                        }
                        IconOnlyButton {
                            Layout.alignment: Qt.AlignVCenter
                            text: qsTr("Resume all")
                            icon.source: QuickUI.faUrlBase + "play"
                            onClicked: SyncthingData.connection.resumeAllDirs()
                            flat: true
                        }
                        IconOnlyButton {
                            Layout.alignment: Qt.AlignVCenter
                            text: qsTr("Rescan all")
                            icon.source: QuickUI.faUrlBase + "refresh"
                            onClicked: SyncthingData.connection.rescanAllDirs()
                            flat: true
                        }
                        IconOnlyButton {
                            text: qsTr("Filter folders")
                            icon.source: QuickUI.faUrlBase + "search"
                            onClicked: foldersFilterField.toggle()
                            flat: true
                        }
                        Item {
                            Layout.preferredWidth: dirsListView.ScrollBar?.vertical.width
                        }
                    }
                    FilterField {
                        id: foldersFilterField
                        view: dirsListView
                    }
                    DirListView {
                        id: dirsListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        anchors.fill: null
                        clip: true
                        stackView: null
                    }
                }
                ColumnLayout {
                    id: rightColumn
                    visible: !mainPane.compact || tabBar.currentIndex === devsButton.tabIndex || tabBar.currentIndex === statsButton.tabIndex
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ColumnLayout {
                        visible: !mainPane.compact || tabBar.currentIndex === statsButton.tabIndex
                        Layout.fillWidth: true
                        Layout.fillHeight: mainPane.compact
                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                Layout.fillWidth: true
                                leftPadding: 10
                                text: qsTr("Statistics")
                                font.weight: Font.Medium
                            }
                            Item {
                                Layout.fillWidth: true
                            }
                            IconOnlyButton {
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("Recent changes")
                                icon.source: QuickUI.faUrlBase + "history"
                                onClicked: QuickUI.showRecentChanges()
                                flat: true
                            }
                            IconOnlyButton {
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("Show ID")
                                icon.source: QuickUI.faUrlBase + "qrcode"
                                onClicked: TrayWidget.showOwnDeviceId()
                                flat: true
                            }
                            IconOnlyButton {
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("Show logs")
                                icon.source: QuickUI.faUrlBase + "terminal"
                                onClicked: TrayWidget.showLog()
                                flat: true
                            }
                            Item {
                                Layout.preferredWidth: devsListView.ScrollBar?.vertical.width
                            }
                        }
                        CustomFlickable {
                            id: statisticsView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: mainPane.compact ? 0 : statisticsLayout.implicitHeight
                            contentHeight: statisticsLayout.height
                            clip: true
                            ScrollBar.vertical: ScrollBar {
                                id: statisticsScrollBar
                            }
                            ScrollIndicator.vertical: null
                            ColumnLayout {
                                id: statisticsLayout
                                width: statisticsView.width - statisticsScrollBar.width
                                StatisticsRow {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 15
                                    Layout.rightMargin: 15
                                    stats: localSyncProgress.stats.global
                                    labelText: qsTr("Global state")
                                    iconName: "globe"
                                }
                                StatisticsRow {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 15
                                    Layout.rightMargin: 15
                                    stats: localSyncProgress.stats.local
                                    labelText: qsTr("Local state")
                                    iconName: "home"
                                }
                                LocalSyncProgress {
                                    id: localSyncProgress
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 15
                                    Layout.rightMargin: 15
                                    spacing: 10
                                }
                                RemoteSyncProgress {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 15
                                    Layout.rightMargin: 15
                                    spacing: 10
                                }
                            }
                        }
                    }
                    ColumnLayout {
                        visible: !mainPane.compact || tabBar.currentIndex === devsButton.tabIndex
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                Layout.fillWidth: true
                                leftPadding: 10
                                text: qsTr("Devices")
                                font.weight: Font.Medium
                            }
                            Item {
                                Layout.fillWidth: true
                            }
                            IconOnlyButton {
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("Add device")
                                icon.source: QuickUI.faUrlBase + "plus"
                                onClicked: QuickUI.editDev("", "", null)
                                flat: true
                            }
                            IconOnlyButton {
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("Pause all")
                                icon.source: QuickUI.faUrlBase + "pause"
                                onClicked: SyncthingData.connection.pauseAllDevs()
                                flat: true
                            }
                            IconOnlyButton {
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("Resume all")
                                icon.source: QuickUI.faUrlBase + "play"
                                onClicked: SyncthingData.connection.resumeAllDevs()
                                flat: true
                            }
                            IconOnlyButton {
                                id: devsFilterButton
                                text: qsTr("Filter devices")
                                icon.source: QuickUI.faUrlBase + "search"
                                onClicked: devsFilterField.toggle()
                                flat: true
                            }
                            Item {
                                Layout.preferredWidth: devsListView.ScrollBar?.vertical.width
                            }
                        }
                        FilterField {
                            id: devsFilterField
                            view: devsListView
                        }
                        DevListView {
                            id: devsListView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            anchors.fill: null
                            clip: true
                            stackView: null
                        }
                    }
                }
            }
            TabBar {
                id: tabBar
                Layout.fillWidth: true
                visible: mainPane.compact
                position: TabBar.Footer
                MainTabButton {
                    id: statsButton
                    text: qsTr("Statistics")
                    iconName: "area-chart"
                    tabIndex: 0
                }
                MainTabButton {
                    id: folderButton
                    text: qsTr("Folders")
                    iconName: "folder"
                    tabIndex: 1
                }
                MainTabButton {
                    id: devsButton
                    text: qsTr("Devices")
                    iconName: "sitemap"
                    tabIndex: 2
                }
            }
        }
        readonly property bool compact: width < 400
    }

    Shortcut {
        sequence: "Ctrl+F"
        onActivated: foldersFilterField.toggle()
    }
    Shortcut {
        sequence: "Ctrl+Shift+F"
        onActivated: devsFilterField.toggle()
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.ForwardButton | Qt.BackButton
        propagateComposedEvents: true
        onClicked: (event) => {
            const button = event.button;
            if (button === Qt.BackButton) {
            } else if (button === Qt.ForwardButton) {
            }
        }
    }

    readonly property Theming theming: Theming {
        currentPage: null
    }
    readonly property Meta meta: Meta {
    }
    readonly property Notifications notifications: Notifications {
        pageStack: null
        appConnections: Connections {}
        uiConnections: Connections {}
        connectionConnections: Connections {}
        notifierConnections: Connections {}
        onOpeningUrlRequested: SyncthingModels.openUrlExternally(url)
    }
}
