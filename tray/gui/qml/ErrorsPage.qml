import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Main

Page {
    title: qsTr("Notifications/errors")
    Component.onCompleted: App.loadErrors(listView)
    actions: [
        Action {
            text: qsTr("Clear")
            icon.source: App.faUrlBase + "trash"
            onTriggered: (source) => App.connection.requestClearingErrors()
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
