import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Page {
    title: qsTr("Notifications/errors")
    Component.onCompleted: app.loadErrors(listView)
    actions: [
        Action {
            text: qsTr("Clear")
            icon.source: app.faUrlBase + "trash"
            onTriggered: (source) => app.connection.requestClearingErrors()
        }
    ]
    ListView {
        id: listView
        anchors.fill: parent
        activeFocusOnTab: true
        keyNavigationEnabled: true
        delegate: ItemDelegate {
            width: listView.width
            onPressAndHold: app.copyText(modelData.message)
            contentItem: GridLayout {
                columns: 2
                columnSpacing: 10
                Image {
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16
                    source: app.faUrlBase + "calendar"
                    width: 16
                    height: 16
                }
                Label {
                    Layout.fillWidth: true
                    text: modelData.when
                    elide: Text.ElideRight
                    font.weight: Font.Light
                }
                Image {
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16
                    source: app.faUrlBase + "exclamation-triangle"
                    width: 16
                    height: 16
                }
                Label {
                    Layout.fillWidth: true
                    text: modelData.message
                    wrapMode: Text.WordWrap
                    font.weight: Font.Light
                }

            }
            required property var modelData
        }
        ScrollIndicator.vertical: ScrollIndicator { }
    }

    required property list<Action> actions
}
