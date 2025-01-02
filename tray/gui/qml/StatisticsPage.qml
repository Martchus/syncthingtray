import QtQuick
import QtQuick.Controls

import Main

ObjectConfigPage {
    id: page
    title: qsTr("Statistics")
    Component.onCompleted: page.loadStatistics()
    actions: [
        Action {
            text: qsTr("Refresh")
            icon.source: App.faUrlBase + "refresh"
            onTriggered: (source) => page.loadStatistics()
        }
    ]
    readOnly: true
    specialEntries: [
        {key: "stConfigDir", type: "readonly", label: qsTr("Syncthing config directory")},
        {key: "stDataDir", type: "readonly", label: qsTr("Syncthing data directory")},
        {key: "stDbSize", type: "readonly", label: qsTr("Syncthing database size")}
    ]
    function loadStatistics() {
        App.loadStatistics((res, error) => {
            page.configObject = res;
            page.model.loadEntries();
        });
    }
}
