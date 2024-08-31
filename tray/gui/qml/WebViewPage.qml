import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import WebViewItem

Page {
    title: qsTr("Syncthing")
    Layout.fillWidth: true
    Layout.fillHeight: true
    WebViewItem {
        id: webViewItem
        anchors.fill: parent
    }
    property alias actions: webViewItem.actions
}
