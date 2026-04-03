import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

ApplicationWindow {
    id: pageWindow
    visible: true
    width: 700
    height: 500
    title: (page.currentItem?.title ?? page.title) + " - WORK IN PROGRESS"
    font: theming.font
    flags: QuickUI.extendedClientArea ? (Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint) : (Qt.Window)
    leftPadding: 0
    rightPadding: 0

    Material.theme: theming.Material.theme
    Material.primary: theming.Material.primary
    Material.accent: theming.Material.accent

    required property var page

    readonly property Theming theming: Theming {
        pageStack: null
    }
    readonly property Meta meta: Meta {
    }
}
