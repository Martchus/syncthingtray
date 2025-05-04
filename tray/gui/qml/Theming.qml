import QtQml
import QtQuick.Controls.Material

import Main

QtObject {
    Material.theme: App.darkmodeEnabled ? Material.Dark : Material.Light
    Material.primary: pageStack.currentPage.isDangerous ? Material.Red : Material.LightBlue
    Material.accent: Material.primary
    Material.onForegroundChanged: App.setPalette(Material.foreground, Material.background)

    // propagate palette of Qt Quick Controls 2 style to regular QPalette of QGuiApplication for icon rendering
    Component.onCompleted: App.setPalette(Material.foreground, Material.background)

    readonly property var font: App.font
    required property PageStack pageStack
}
