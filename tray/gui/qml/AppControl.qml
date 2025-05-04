import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import Qt.labs.qmlmodels

import Main

Control {
    id: control
    anchors.fill: parent
    font: theming.font
    Material.theme: theming.Material.theme
    Material.primary: theming.Material.primary
    Material.accent: theming.Material.accent
    onVisibleChanged: App.setCurrentControls(window.visible, pageStack.currentIndex)

    LeftDrawer {
        id: drawer
        pageStack: pageStack
        aboutDialog: aboutDialog
        closeDialog: closeDialog
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        MainToolBar {
            Layout.fillWidth: true
            Layout.leftMargin: drawer.visible ? drawer.effectiveWidth : 0
            drawer: drawer
            pageStack: pageStack
        }
        PageStack {
            id: pageStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: drawer.visible ? drawer.effectiveWidth : 0
            window: control
            onChangesMightBeDiscarded: discardChangesDialog.open()
        }
        MainTabBar {
            id: toolBar
            Layout.fillWidth: true
            drawer: drawer
            pageStack: pageStack
        }
    }
    AboutDialog {
        id: aboutDialog
    }
    CloseDialog {
        id: closeDialog
        meta: control.meta
        onCloseRequested: App.quit()
    }
    DiscardChangesDialog {
        id: discardChangesDialog
        meta: control.meta
        pageStack: pageStack
    }
    readonly property Notifications notifications: Notifications {
        pageStack: pageStack
    }
    readonly property Theming theming: Theming {
        pageStack: pageStack
    }
    readonly property Meta meta: Meta {
    }
}
