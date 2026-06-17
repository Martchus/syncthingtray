import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main
import Tray

Pane {
    id: mainPane
    padding: 0
    anchors.fill: parent
    Material.theme: theming.Material.theme
    Material.primary: theming.Material.primary
    Material.accent: theming.Material.accent
    background: Rectangle {
        color: mainPane.palette.base
    }
    ColumnLayout {
        anchors.fill: parent
        StackLayout {
            id: pageStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            onCurrentIndexChanged: TrayWidget.handleCurrentTabChanged(pageStack.currentIndex)
            DirListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                anchors.fill: null
                stackView: null
            }
            DevListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                anchors.fill: null
                stackView: null
            }
            ChangesListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                anchors.fill: null
            }
            function setCurrentIndex(index) {
                pageStack.currentIndex = index;
            }
        }
        Pane {
            padding: 0
            Layout.fillWidth: true
            TabBar {
                anchors.fill: parent
                position: TabBar.Footer
                MainTabButton {
                    text: qsTr("Folders")
                    iconName: "folder"
                    tabIndex: 0
                    displayWithIcon: AbstractButton.TextBesideIcon
                }
                MainTabButton {
                    text: qsTr("Devices")
                    iconName: "sitemap"
                    tabIndex: 1
                    displayWithIcon: AbstractButton.TextBesideIcon
                }
                MainTabButton {
                    text: qsTr("Recent changes")
                    iconName: "history"
                    tabIndex: 2
                    displayWithIcon: AbstractButton.TextBesideIcon
                }
            }
        }
    }
    readonly property Theming theming: Theming {
        currentPage: null
    }
}
