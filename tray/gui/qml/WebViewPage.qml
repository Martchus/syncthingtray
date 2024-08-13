import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
//import QtWebView

Page {
    title: qsTr("Syncthing")
    Layout.fillWidth: true
    Layout.fillHeight: true
    ColumnLayout {
        Label {
            text: "TODO: implement web view"
        }
        /*
        Button {
            text: "Refresh"
            onClicked: webView.url = app.connection.makeUrlWithCredentials()
        }
        WebView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            id: webView
        }
        */
    }
}
