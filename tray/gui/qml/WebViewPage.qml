import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Main
import WebViewItem

Page {
    id: webViewPage
    title: qsTr("Web-based UI")
    Layout.fillWidth: true
    Layout.fillHeight: true
    WebViewItem {
        id: webViewItem
        anchors.fill: parent
        url: webViewPage.active ? App.connection.syncthingUrlWithCredentials : "about:blank"
    }
    property alias actions: webViewItem.actions
    property bool active: false
}
