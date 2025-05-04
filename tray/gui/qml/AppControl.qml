import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import Qt.labs.qmlmodels

import Main

Control {
    id: control
    font: main.font
    Material.theme: main.Material.theme
    Material.primary: main.Material.primary
    Material.accent: main.Material.accent

    LeftDrawer {
        id: drawer
        pageStack: pageStack
    }
    ColumnLayout {
        anchors.fill: parent
        PageStack {
            id: pageStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: drawer.visible ? drawer.effectiveWidth : 0
            window: control
        }
    }

    readonly property Main main: Main {
        window: control
        pageStack: pageStack
        drawer: drawer
    }
}
