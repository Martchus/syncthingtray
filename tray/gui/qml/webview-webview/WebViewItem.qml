import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtWebView

ColumnLayout {
    id: webViewItem
    WebView {
        id: webView
        Layout.fillWidth: true
        Layout.fillHeight: true
        onLoadingChanged: (request) => {
            errorLabel.visible = request.errorString.length > 0;
            errorLabel.text = request.errorString;
        }
        BusyIndicator {
            anchors.centerIn: parent
            running: webView.loading
        }
        Label {
            id: errorLabel
            anchors.centerIn: parent
            visible: false
        }
    }
    property alias url: webView.url
    property list<Action> actions: [
        Action {
            text: qsTr("Refresh")
            icon.source: app.faUrlBase + "refresh"
            onTriggered: webView.reload()
        },
        Action {
            text: qsTr("Open in web browser")
            icon.source: app.faUrlBase + "external-link"
            onTriggered: Qt.openUrlExternally(webViewItem.url)
        }
    ]
}
