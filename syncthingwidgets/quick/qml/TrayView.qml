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
        Keys.onLeftPressed: (event) => {
            tabBar.setCurrentIndex(tabBar.currentIndex > 0 ? tabBar.currentIndex - 1 : tabBar.count - 1);
            pageStack.setCurrentIndex(tabBar.currentItem.tabIndex);
        }
        Keys.onRightPressed: (event) => {
            tabBar.setCurrentIndex(tabBar.currentIndex < tabBar.count - 1 ? tabBar.currentIndex + 1 : 0);
            pageStack.setCurrentIndex(tabBar.currentItem.tabIndex);
        }
        StackLayout {
            id: pageStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            onCurrentIndexChanged: TrayWidget.handleCurrentTabChanged(pageStack.currentIndex)
            DirListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                anchors.fill: null
                focus: StackLayout.isCurrentItem
                stackView: null
            }
            DevListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                anchors.fill: null
                focus: StackLayout.isCurrentItem
                stackView: null
            }
            ChangesListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                focus: StackLayout.isCurrentItem
                anchors.fill: null
            }
            function setCurrentIndex(index) {
                pageStack.currentIndex = Math.min(Math.max(0, index), pageStack.count - 1);
            }
        }
        Pane {
            padding: 0
            Layout.fillWidth: true
            TabBar {
                id: tabBar
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
