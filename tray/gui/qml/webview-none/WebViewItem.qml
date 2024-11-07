import QtQuick
import QtQuick.Controls

import Main

Label {
    id: webViewItem
    anchors.fill: parent
    text: qsTr("The app has not been built with web view support so this page is not available.")
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    wrapMode: Text.WordWrap
    property url url
    property list<Action> actions: [
        Action {
            text: qsTr("Open in web browser")
            icon.source: App.faUrlBase + "external-link"
            onTriggered: Qt.openUrlExternally(webViewItem.url)
        }
    ]
}
