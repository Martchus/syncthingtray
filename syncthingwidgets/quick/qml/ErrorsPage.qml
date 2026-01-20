import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

Page {
    title: qsTr("Notifications/errors")
    Component.onCompleted: SyncthingModels.loadErrors(listView)
    actions: [
        Action {
            text: qsTr("Clear")
            icon.source: QuickUI.faUrlBase + "trash"
            onTriggered: SyncthingData.connection.requestClearingErrors()
        }
    ]
    CustomListView {
        id: listView
        anchors.fill: parent
        delegate: ErrorsDelegate {}
    }

    required property list<Action> actions
}
