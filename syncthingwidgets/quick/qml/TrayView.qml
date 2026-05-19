import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main
import Tray

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
            ScrollBar.vertical: ScrollBar { }
            ScrollIndicator.vertical: null
            anchors.fill: null
            stackView: null
        }
        DevListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ScrollBar.vertical: ScrollBar { }
            ScrollIndicator.vertical: null
            anchors.fill: null
            stackView: null
        }
        ChangesListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ScrollBar.vertical: ScrollBar { }
            ScrollIndicator.vertical: null
            anchors.fill: null
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
