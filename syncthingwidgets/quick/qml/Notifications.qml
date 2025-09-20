import QtQuick

import Main

QtObject {
    readonly property Connections appConnections: Connections {
        target: App
        function onError(message) {
            showNotifiction(message);
        }
        function onInfo(message) {
            showNotifiction(message);
        }
        function onInternalError(error) {
            showNotifiction(error.message);
        }
        function onTextShared(text) {
            if (text.match(/^[0-9A-z]{7}(-[0-9A-z]{7}){7}$/)) {
                pageStack.addDevice(text);
            } else {
                showNotifiction(qsTr("Not a valid device ID."));
            }
        }
        function onNewDeviceTriggered(devId) {
            pageStack.addDevice(devId);
        }
        function onNewDirTriggered(devId, dirId, dirLabel) {
            pageStack.addDir(dirId, dirLabel, [devId]);
        }
    }
    readonly property Connections connectionConnections: Connections {
        target: App.connection
        function onNewConfigTriggered() {
            showNotifiction(qsTr("Configuration changed"));
        }
    }
    readonly property Connections notifierConnections: Connections {
        target: App.notifier
        function onDisconnected() {
            showNotifiction(qsTr("UI disconnected from Syncthing backend"));
        }
    }

    signal notification(message: string)
    function showNotifiction(message) {
        return App.showToast(message) || notification(message);
    }

    required property PageStack pageStack
}
