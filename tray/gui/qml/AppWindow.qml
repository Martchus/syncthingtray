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
    title: meta.title
    font: theming.font
    header: MainToolBar {
        drawer: drawer
        pageStack: pageStack
        leftMargin: pageStack.anchors.leftMargin
    }
    footer: MainTabBar {
        id: toolBar
        drawer: drawer
        pageStack: pageStack
    }
    Material.theme: theming.Material.theme
    Material.primary: theming.Material.primary
    Material.accent: theming.Material.accent
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
    onVisibleChanged: App.setCurrentControls(appWindow.visible, pageStack.currentIndex)
    onActiveFocusItemChanged: {
        if (activeFocusItem?.toString().startsWith("QQuickPopupItem")) {
            appWindow.contentItem.forceActiveFocus(Qt.ActiveWindowFocusReason);
        }
    }
    onClosing: (event) => {
        if (!appWindow.forceClose && App.launcher.running) {
            event.accepted = false;
            closeDialog.open();
        }
    }

    LeftDrawer {
        id: drawer
        pageStack: pageStack
        aboutDialog: aboutDialog
        closeDialog: closeDialog
    }
    PageStack {
        id: pageStack
        anchors.fill: parent
        anchors.leftMargin: drawer.visible ? drawer.effectiveWidth : 0
        window: appWindow
        onChangesMightBeDiscarded: discardChangesDialog.open()
    }
    AboutDialog {
        id: aboutDialog
    }
    CloseDialog {
        id: closeDialog
        meta: appWindow.meta
        onCloseRequested: (appWindow.forceClose = true) && appWindow.close()
    }
    DiscardChangesDialog {
        id: discardChangesDialog
        meta: appWindow.meta
        pageStack: pageStack
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.ForwardButton | Qt.BackButton
        propagateComposedEvents: true
        onClicked: (event) => {
            const button = event.button;
            if (button === Qt.BackButton) {
                pageStack.pop();
            } else if (button === Qt.ForwardButton) {
                pageStack.forward();
            }
        }
    }
    ToolTip {
        anchors.centerIn: Overlay.overlay
        id: notifictionToolTip
        timeout: 5000
    }
    readonly property Notifications notifications: Notifications {
        pageStack: pageStack
        onNotification: {
            notifictionToolTip.text = notifictionToolTip.visible ? `${notifictionToolTip.text}\n${message}` : message;
            notifictionToolTip.open();
        }
    }
    readonly property Theming theming: Theming {
        pageStack: pageStack
    }
    readonly property Meta meta: Meta {
    }
    property bool forceClose: false
}
