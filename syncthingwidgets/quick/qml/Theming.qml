import QtQml
import QtQuick.Controls.Material

import Main

QtObject {
    Material.theme: QuickUI.darkmodeEnabled ? Material.Dark : Material.Light
    Material.primary: pageStack.currentPage.isDangerous ? Material.Red : Material.LightBlue
    Material.accent: QuickUI.darkmodeEnabled ? Material.color(pageStack.currentPage.isDangerous ? Material.Red : Material.LightBlue, Material.Shade200) : Material.primary
    Material.onForegroundChanged: QuickUI.setPalette(Material.foreground, Material.background)

    // propagate palette of Qt Quick Controls 2 style to regular QPalette of QGuiApplication for icon rendering
    Component.onCompleted: QuickUI.setPalette(Material.foreground, Material.background)

    readonly property var font: QuickUI.font
    required property PageStack pageStack
}
