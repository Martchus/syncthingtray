import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Page {
    title: qsTr("Folder overview")
    Layout.fillWidth: true
    Layout.fillHeight: true
    ListView {
        anchors.fill: parent
        model: DelegateModel {
            model: app.dirModel
            delegate: ItemDelegate {
                width: parent.width
                text: name
            }
        }
        ScrollIndicator.vertical: ScrollIndicator { }
    }
}
