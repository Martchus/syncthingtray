import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import Qt.labs.qmlmodels

import Main

Control {
    id: control
    font: App.font
    onVisibleChanged: App.setCurrentControls(control.visible, 0 /* pageStack.currentIndex */)
    Material.theme: App.darkmodeEnabled ? Material.Dark : Material.Light
    //Material.primary: pageStack.currentPage.isDangerous ? Material.Red : Material.LightBlue
    Material.primary: Material.LightBlue
    Material.accent: Material.primary
    Material.onForegroundChanged: App.setPalette(Material.foreground, Material.background)
 
    Label {
        anchors.fill: parent
        text: qsTr("foobar")
    }
}
