import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

ColumnLayout {
    anchors.fill: parent
    StackLayout {
        id: pageStack
        Layout.fillWidth: true
        Layout.fillHeight: true
        DirListView {
            id: dirsListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            anchors.fill: null
            stackView: null
        }
        DevListView {
            id: devsListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            anchors.fill: null
            stackView: null
        }
        Label {
            id: changesPage
            text: "WIP: changes"
        }
        function setCurrentIndex(index) {
            pageStack.currentIndex = index;
        }
    }
    TabBar {
        position: TabBar.Footer
        Layout.fillWidth: true
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
