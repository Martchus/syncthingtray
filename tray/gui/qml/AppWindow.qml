import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import Qt.labs.qmlmodels

import Main

ApplicationWindow {
    id: appWindow
    visible: true
    width: 700
    height: 500
    title: main.title
    font: main.font
    header: main.toolBar
    footer: main.tabBar
    Material.theme: main.Material.theme
    Material.primary: main.Material.primary
    Material.accent: main.Material.accent
    Component.onCompleted: {
        // handle global keyboard and mouse events
        appWindow.contentItem.forceActiveFocus(Qt.ActiveWindowFocusReason);
        appWindow.contentItem.Keys.released.connect((event) => {
            const key = event.key;
            if (key === Qt.Key_Back || (key === Qt.Key_Backspace && typeof activeFocusItem.getText !== "function")) {
                event.accepted = pageStack.pop();
            } else if (key === Qt.Key_Forward) {
                event.accepted = pageStack.forward();
            } else if (key === Qt.Key_F5) {
                event.accepted = App.reloadMain();
            }
        });
    }
    onActiveFocusItemChanged: {
        if (activeFocusItem?.toString().startsWith("QQuickPopupItem")) {
            appWindow.contentItem.forceActiveFocus(Qt.ActiveWindowFocusReason);
        }
    }
    onClosing: (event) => {
        if (!main.forceClose && App.launcher.running) {
            event.accepted = false;
            main.closeDialog.open();
        }
    }

    LeftDrawer {
        id: drawer
        pageStack: pageStack
    }
    PageStack {
        id: pageStack
        anchors.fill: parent
        anchors.leftMargin: drawer.visible ? drawer.effectiveWidth : 0
        window: appWindow
    }

    readonly property Main main: Main {
        window: appWindow
        pageStack: pageStack
        drawer: drawer
    }
}
