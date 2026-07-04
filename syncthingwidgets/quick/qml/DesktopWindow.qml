import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main
import Tray

ApplicationWindow {
    id: appWindow
    visible: true
    width: 700
    height: 500
    title: meta.title + " - WORK IN PROGRESS"
    font: theming.font
    flags: QuickUI.extendedClientArea ? (Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint) : (Qt.Window)
    leftPadding: 0
    rightPadding: 0
    onVisibleChanged: TrayWidget.handleMainWindowVisibleChanged(appWindow.visible)
    header: Pane {
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
                        text: qsTr("&Home page")
                        onTriggered: SyncthingModels.openUrlExternally("https://syncthing.net")
                    }
                    Action {
                        text: qsTr("&Documentation")
                        onTriggered: SyncthingModels.openUrlExternally("https://docs.syncthing.net")
                    }
                    Action {
                        text: qsTr("&Support")
                        onTriggered: SyncthingModels.openUrlExternally("https://forum.syncthing.net")
                    }
                    Action {
                        text: qsTr("&Changelog")
                        onTriggered: SyncthingModels.openUrlExternally("https://github.com/syncthing/syncthing/releases")
                    }
                    Action {
                        text: qsTr("&Statistics")
                        onTriggered: SyncthingModels.openUrlExternally("https://data.syncthing.net")
                    }
                    Action {
                        text: qsTr("&Bugs")
                        onTriggered: SyncthingModels.openUrlExternally("https://github.com/syncthing/syncthing/issues")
                    }
                    Action {
                        text: qsTr("&Source code")
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
                        text: qsTr("&Documentation")
                        onTriggered: SyncthingModels.openUrlExternally(SyncthingData.documentationUrl)
                    }
                    Action {
                        text: qsTr("&Source code")
                        onTriggered: SyncthingModels.openUrlExternally(SyncthingData.readmeUrl.replace(/\/blob\/.*/, ""))
                    }
                    Action {
                        text: qsTr("&About")
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
                    }
                    Action {
                        text: qsTr("&Web-based UI")
                        onTriggered: TrayWidget.showSyncthingUI(true)
                    }
                    Action {
                        text: qsTr("&Recent changes")
                        onTriggered: QuickUI.showRecentChanges()
                    }
                    Action {
                        text: qsTr("&Show ID")
                        onTriggered: TrayWidget.showOwnDeviceId()
                    }
                    Action {
                        text: qsTr("&Logs")
                        onTriggered: TrayWidget.showLog()
                    }
                    Action {
                        text: qsTr("&Support Bundle")
                        enabled: false
                    }
                    Action {
                        text: qsTr("&Restart")
                        onTriggered: SyncthingData.connection.restart()
                    }
                    Action {
                        text: qsTr("&Shutdown")
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
                    }
                    Action {
                        text: qsTr("&Wizard")
                        onTriggered: TrayWidget.showWizard()
                    }
                }
            }
        }
    }

    Material.theme: theming.Material.theme
    Material.primary: theming.Material.primary
    Material.accent: theming.Material.accent

    Pane {
        anchors.fill: parent
        anchors.leftMargin: parent.SafeArea.margins.left
        anchors.rightMargin: parent.SafeArea.margins.right
        padding: 0
        topPadding: 10
        background: Rectangle {
            color: appWindow.palette.base
        }
        GridLayout {
            anchors.fill: parent
            columns: width > 400 ? 2 : 1
            uniformCellWidths: true
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        leftPadding: 5
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
                        text: qsTr("Filter")
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
                Layout.fillWidth: true
                Layout.fillHeight: true
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        leftPadding: 5
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
                Statistics {
                    stats: localSyncProgress.stats.global
                    labelText: qsTr("Global state")
                    iconName: "globe"
                    dense: true
                    highlighted: false
                    hoverEnabled: false
                }
                Statistics {
                    stats: localSyncProgress.stats.local
                    labelText: qsTr("Local state")
                    iconName: "home"
                    dense: true
                    highlighted: false
                    hoverEnabled: false
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
                        text: qsTr("Filter")
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
