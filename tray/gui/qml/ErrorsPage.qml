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
        delegate: ErrorsDelegate {}
        ScrollIndicator.vertical: ScrollIndicator { }
    }

    required property list<Action> actions
}
