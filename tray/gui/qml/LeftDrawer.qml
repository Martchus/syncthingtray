import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

Drawer {
    id: drawer
    width: Math.min(0.66 * parent.width, 200)
    height: parent.height
    interactive: inPortrait || parent.width < 600
    modal: interactive
    position: initialPosition
    visible: !interactive

    CustomListView {
        id: drawerListView
        anchors.fill: parent
        footer: ColumnLayout {
            width: parent.width
            ItemDelegate {
                Layout.fillWidth: true
                text: Qt.application.version
                icon.source: App.faUrlBase + "info-circle"
                icon.width: App.iconSize
                icon.height: App.iconSize
                onClicked: aboutDialog.visible = true
            }
            ItemDelegate {
                Layout.fillWidth: true
                text: qsTr("Quit")
                icon.source: App.faUrlBase + "power-off"
                icon.width: App.iconSize
                icon.height: App.iconSize
                onClicked: closeDialog.visible = true
            }
        }
        model: ListModel {
            ListElement {
                name: qsTr("Start")
                iconName: "home"
            }
            ListElement {
                name: qsTr("Folders")
                iconName: "folder"
            }
            ListElement {
                name: qsTr("Devices")
                iconName: "sitemap"
            }
            ListElement {
                name: qsTr("Recent changes")
                iconName: "history"
            }
            ListElement {
                name: qsTr("Advanced")
                iconName: "cogs"
            }
            ListElement {
                name: qsTr("App settings")
                iconName: "cog"
            }
        }
        delegate: ItemDelegate {
            id: drawerDelegate
            text: name
            activeFocusOnTab: true
            icon.source: App.faUrlBase + iconName
            icon.width: App.iconSize
            icon.height: App.iconSize
            width: parent.width
            onClicked: {
                pageStack.setCurrentIndex(index);
                drawer.position = drawer.initialPosition;
            }
        }
    }

    required property PageStack pageStack
    required property AboutDialog aboutDialog
    required property CloseDialog closeDialog
    readonly property bool inPortrait: parent.width < parent.height
    readonly property double initialPosition: interactive ? 0 : 1
    readonly property int effectiveWidth: !interactive ? width : 0
}
