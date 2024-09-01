import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Page {
    title: qsTr("App settings")
    Layout.fillWidth: true
    Layout.fillHeight: true
    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 10
        GridLayout {
            width: scrollView.width
            columns: 2
            columnSpacing: 5
            rowSpacing: 5
            Label {
                text: qsTr("Syncthing URL")
            }
            TextField {
                id: syncthingUrlTextField
                Layout.fillWidth: true
                text: app.settings.syncthingUrl
            }
            Label {
                text: qsTr("API key")
            }
            TextField {
                id: apiKeyTextField
                Layout.fillWidth: true
                text: app.settings.apiKey
            }
        }
    }
    property list<Action> actions: [
        Action {
            text: qsTr("Apply")
            icon.source: app.faUrlBase + "check"
            onTriggered: (source) => {
                app.settings.syncthingUrl = syncthingUrlTextField.text
                app.settings.apiKey = apiKeyTextField.text
                app.applySettings()
            }
        }
    ]

}
