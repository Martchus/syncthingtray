import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Page {
    title: qsTr("Recent changes")
    Layout.fillWidth: true
    Layout.fillHeight: true
    ListView {
        anchors.fill: parent
        model: DelegateModel {
            model: app.changesModel
            delegate: ItemDelegate {
                text: path
            }
        }
        ScrollIndicator.vertical: ScrollIndicator { }
    }
}
