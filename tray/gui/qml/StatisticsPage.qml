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
        {key: "uptime", label: qsTr("Uptime")},
        {key: "memoryUsage", label: qsTr("Memory usage (only Go runtime)")},
        {key: "processRSS", label: qsTr("Resident set size (only Go runtime)")},
        {key: "natType", label: qsTr("NAT type")},
        {key: "stConfigDir", label: qsTr("Syncthing config directory")},
        {key: "stDataDir", label: qsTr("Syncthing data directory")},
        {key: "stLevelDbSize", label: qsTr("Syncthing database size (LevelDB)"), optional: true},
        {key: "stLevelDbMigratedSize", label: qsTr("Syncthing database size (LevelDB, migrated)"), optional: true},
        {key: "stSQLiteDbSize", label: qsTr("Syncthing database size (SQLite)"), optional: true},
        {key: "extFilesDir", label: qsTr("External files directory"), optional: true},
        {key: "extStoragePaths", label: qsTr("External storage paths"), optional: true},
        {key: "numCPU", label: qsTr("CPU threads of system")},
        {key: "memorySize", label: qsTr("Memory size of system")},
        {key: "numDevices", label: qsTr("Number of Syncthing devices")},
        {key: "numFolders", label: qsTr("Number of Syncthing folders")},
        {key: "totFiles", label: qsTr("Number of files managed by Syncthing")},
        {key: "totSize", label: qsTr("Size of files managed by Syncthing")},
    ]
    function loadStatistics() {
        page.isLoading = true;
        App.loadStatistics((res, error) => {
            // delete unwanted or empty statistics
            delete res.version;
            Object.entries(res).forEach((entry) => {
                const value = entry[1];
                const type = typeof value;
                if (type === "array" || (type !== "object" && value?.toString() === "")) {
                    delete res[entry[0]];
                }
            });

            page.isLoading = false;
            page.configObject = res;
            page.model.loadEntries();
        });
    }
}
