import QtQml
import QtQuick
import QtQuick.Controls.Material


Instantiator {
    model: 1
    onObjectAdded: (index, object) => menu.insertItem(index, object)
    onObjectRemoved: (index, object) => menu.removeItem(object)
    delegate: MenuSeparator {
    }
    required property Menu menu
}
