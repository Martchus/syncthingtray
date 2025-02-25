import QtQuick
import QtQuick.Controls.Material

import Main

ObjectConfigPage {
    id: page
    title: qsTr("Statistics")
    isLoading: true
    Component.onCompleted: page.loadStatistics()
    actions: [
        Action {
            text: qsTr("Refresh")
            icon.source: App.faUrlBase + "refresh"
            onTriggered: page.loadStatistics()
        }
    ]
    readOnly: true
    specialEntries: [
        {key: "platform", label: qsTr("Platform")},
        {key: "longVersion", label: qsTr("Syncthing version")},
        {key: "memoryUsageMiB", label: qsTr("Memory usage in MiB")},
        {key: "natType", label: qsTr("NAT type")},
        {key: "stConfigDir", label: qsTr("Syncthing config directory")},
        {key: "stDataDir", label: qsTr("Syncthing data directory")},
        {key: "stDbSize", label: qsTr("Syncthing database size")},
    ]
    function loadStatistics() {
        page.isLoading = true;
        App.loadStatistics((res, error) => {
            // delete unwanted or empty statistics
            delete res.version;
            Object.entries(res).forEach((entry) => {
                const value = entry[1];
                const type = typeof value;
                if (type === "array" || (type !== "object" && value.toString() === "")) {
                    delete res[entry[0]];
                }
            });

            page.isLoading = false;
            page.configObject = res;
            page.model.loadEntries();
        });
    }
}
