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
        delegate: ErrorsDelegate {}
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
