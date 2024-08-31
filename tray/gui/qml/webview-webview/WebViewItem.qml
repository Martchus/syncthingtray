import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtWebView

ColumnLayout {
    RowLayout {
        Layout.margins: 10
        Label {
            id: addressBar
            Layout.fillWidth: true
            text: app.connection.syncthingUrl
            elide: Label.ElideRight
        }
    }
    WebView {
        id: webView
        Layout.fillWidth: true
        Layout.fillHeight: true
        url: app.connection.syncthingUrlWithCredentials
        onLoadingChanged: (request) => {
            addressBar.text = request.url;
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
    property list<Action> actions: [
        Action {
            text: "Refresh"
            icon.source: app.faUrlBase + "refresh"
            onTriggered: webView.reload()
        }
    ]
}
