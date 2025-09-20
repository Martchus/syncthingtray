import QtQml
import QtQuick
import QtQuick.Controls.Material

Instantiator {
    onObjectAdded: (index, object) => menu.insertItem(index, object)
    onObjectRemoved: (index, object) => menu.removeItem(object)
    delegate: MenuItem {
        required property Action modelData
        text: modelData.text
        enabled: modelData.enabled
        visible: enabled
        height: visible ? implicitHeight : 0
        icon.source: modelData.icon.source
        icon.width: App.iconSize
        icon.height: App.iconSize
        onTriggered: modelData?.trigger()
    }
    required property Menu menu
}
