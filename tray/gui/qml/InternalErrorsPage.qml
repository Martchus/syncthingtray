import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Page {
    title: qsTr("Log of Syncthing API errors")
    Layout.fillWidth: true
    Layout.fillHeight: true
    actions: [
        Action {
            text: qsTr("Clear")
            icon.source: app.faUrlBase + "trash"
            onTriggered: (source) => {
                app.clearInternalErrors();
                listView.model = app.internalErrors();
            }
        }
    ]
    ListView {
        id: listView
        anchors.fill: parent
        activeFocusOnTab: true
        keyNavigationEnabled: true
        model: app.internalErrors()
        delegate: ItemDelegate {
            width: listView.width
            onPressAndHold: app.copyText(`${modelData.when}\n${modelData.url}: ${modelData.message}`)
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
    Connections {
        target: app
        function onInternalError(error) {
            listView.model.push(error)
        }
    }
    property list<Action> actions
}
