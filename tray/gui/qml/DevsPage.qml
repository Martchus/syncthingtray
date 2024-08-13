import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Page {
    title: qsTr("Device overview")
    Layout.fillWidth: true
    Layout.fillHeight: true
    ListView {
        anchors.fill: parent
        model: DelegateModel {
            model: app.devModel
            delegate: ItemDelegate {
                width: parent.width
                text: name
            }
        }
        ScrollIndicator.vertical: ScrollIndicator { }
    }
}
