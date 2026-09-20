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
        iconNameTheme: "go-home-symbolic"
        tabIndex: 0
    }
    MainTabButton {
        text: qsTr("Folders")
        iconName: "folder"
        iconNameTheme: "folder-symbolic"
        tabIndex: 1
    }
    MainTabButton {
        text: qsTr("Devices")
        iconName: "sitemap"
        iconNameTheme: "preferences-system-network-symbolic"
        tabIndex: 2
    }
    MainTabButton {
        text: qsTr("Recent changes")
        iconName: "history"
        iconNameTheme: "view-history"
        tabIndex: 3
    }
    MainTabButton {
        text: qsTr("More")
        iconName: "cog"
        iconNameTheme: "applications-preferences-symbolic"
        tabIndex: 5
    }
    required property LeftDrawer drawer
    required property PageStack pageStack
}
