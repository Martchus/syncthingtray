import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

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
    header: RowLayout {
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
                Action {
                    text: qsTr("&Introduction")
                    icon.source: QuickUI.faUrlBase + "info-circle"
                    icon.width: QuickUI.iconSize
                    icon.height: QuickUI.iconSize
                }
                MenuSeparator {
                }
                Action {
                    text: qsTr("&Home page")
                    icon.source: QuickUI.faUrlBase + "home"
                    icon.width: QuickUI.iconSize
                    icon.height: QuickUI.iconSize
                }
                Action {
                    text: qsTr("&Documentation")
                }
                Action {
                    text: qsTr("&Support")
                }
                MenuSeparator {
                }
                Action {
                    text: qsTr("&Changelog")
                }
                Action {
                    text: qsTr("&Statistics")
                }
                MenuSeparator {
                }
                Action {
                    text: qsTr("&Bugs")
                }
                Action {
                    text: qsTr("&Changelog")
                }
                Action {
                    text: qsTr("&About")
                }
            }
            Menu {
                title: qsTr("&Actions")
                icon.source: QuickUI.faUrlBase + "cog"
                icon.width: QuickUI.iconSize
                icon.height: QuickUI.iconSize
                Action {
                    text: qsTr("&Settings")
                }
                Action {
                    text: qsTr("&Advanced")
                }
                MenuSeparator {
                }
                Action {
                    text: qsTr("&Show ID")
                }
                Action {
                    text: qsTr("&Logs")
                }
                Action {
                    text: qsTr("&Support Bundle")
                }
                MenuSeparator {
                }
                Action {
                    text: qsTr("&Restart")
                }
                Action {
                    text: qsTr("&Shutdown")
                }
            }
        }
    }

    Material.theme: theming.Material.theme
    Material.primary: theming.Material.primary
    Material.accent: theming.Material.accent

    GridLayout {
        anchors.fill: parent
        anchors.leftMargin: parent.SafeArea.margins.left
        anchors.rightMargin: parent.SafeArea.margins.right
        columns: width > 400 ? 2 : 1
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Label {
                Layout.fillWidth: true
                topPadding: 10
                leftPadding: 10
                text: qsTr("Folders")
                font.weight: Font.Medium
            }
            DirListView {
                id: dirsListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                anchors.fill: null
                clip: true
                mainModel: SyncthingModels.sortFilterDirModel
                stackView: null
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Label {
                Layout.fillWidth: true
                topPadding: 10
                leftPadding: 10
                text: qsTr("Devices")
                font.weight: Font.Medium
            }
            DevListView {
                id: devsListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                anchors.fill: null
                clip: true
                mainModel: SyncthingModels.sortFilterDevModel
                stackView: null
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
        pageStack: null
    }
    readonly property Meta meta: Meta {
    }
}
