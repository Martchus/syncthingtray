import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

Page {
    title: qsTr("Log of Syncthing API errors")
    Layout.fillWidth: true
    Layout.fillHeight: true
    actions: [
        Action {
            text: qsTr("Clear")
            icon.source: App.faUrlBase + "trash"
            onTriggered: {
                App.clearInternalErrors();
                listView.model = App.internalErrors();
            }
        }
    ]
    CustomListView {
        id: listView
        anchors.fill: parent
        model: App.internalErrors()
        delegate: ErrorsDelegate {}
    }
    Connections {
        target: App
        function onInternalError(error) {
            listView.model.push(error)
        }
    }
    property list<Action> actions
}
