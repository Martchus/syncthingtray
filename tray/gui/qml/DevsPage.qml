import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Page {
    title: qsTr("Device overview")
    Layout.fillWidth: true
    Layout.fillHeight: true
    DevListView {
        mainModel: app.devModel
    }
}
