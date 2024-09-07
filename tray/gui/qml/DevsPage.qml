import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

StackView {
    id: stackView
    Layout.fillWidth: true
    Layout.fillHeight: true
    initialItem: Page {
        title: qsTr("Devices")
        Layout.fillWidth: true
        Layout.fillHeight: true
        DevListView {
            mainModel: app.devModel
            stackView: stackView
        }
    }
}
