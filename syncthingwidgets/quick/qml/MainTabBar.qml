import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import Qt.labs.qmlmodels

import Main

TabBar {
    visible: drawer.interactive
    currentIndex: Math.min(pageStack.currentIndex, 4)
    MainTabButton {
        text: qsTr("Start")
        iconName: "home"
        tabIndex: 0
    }
    MainTabButton {
        text: qsTr("Folders")
        iconName: "folder"
        tabIndex: 1
    }
    MainTabButton {
        text: qsTr("Devices")
        iconName: "sitemap"
        tabIndex: 2
    }
    MainTabButton {
        text: qsTr("Recent changes")
        iconName: "history"
        tabIndex: 3
    }
    MainTabButton {
        text: qsTr("More")
        iconName: "cog"
        tabIndex: 5
    }
    required property LeftDrawer drawer
    required property PageStack pageStack
}
