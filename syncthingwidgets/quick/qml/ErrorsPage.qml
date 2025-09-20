import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

Page {
    title: qsTr("Notifications/errors")
    Component.onCompleted: App.loadErrors(listView)
    actions: [
        Action {
            text: qsTr("Clear")
            icon.source: App.faUrlBase + "trash"
            onTriggered: App.connection.requestClearingErrors()
        }
    ]
    CustomListView {
        id: listView
        anchors.fill: parent
        delegate: ErrorsDelegate {}
    }

    required property list<Action> actions
}
