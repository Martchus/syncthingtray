import QtQuick

import Main

QtObject {
    property Connections appConnections: Connections {
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
    property Connections uiConnections: Connections {
        target: QuickUI
        function onError(message) {
            showNotifiction(message);
        }
        function onOpeningUrlRequested(url) {
            openingUrlRequested(url);
        }
    }
    property Connections connectionConnections: Connections {
        target: SyncthingData.connection
        function onNewConfigTriggered() {
            showNotifiction(qsTr("Configuration changed"));
        }
    }
    property Connections notifierConnections: Connections {
        target: SyncthingData.notifier
        function onDisconnected() {
            showNotifiction(qsTr("UI disconnected from Syncthing backend"));
        }
    }

    signal notification(message: string)
    signal openingUrlRequested(url: url)

    function showNotifiction(message) {
        return QuickUI.showToast(message) || notification(message);
    }

    required property PageStack pageStack
}
