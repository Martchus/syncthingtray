import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import Qt.labs.qmlmodels

import Main

Item {
    id: main
    onVisibleChanged: App.setCurrentControls(window.visible, pageStack.currentIndex)
    Material.theme: App.darkmodeEnabled ? Material.Dark : Material.Light
    Material.primary: pageStack.currentPage.isDangerous ? Material.Red : Material.LightBlue
    Material.accent: Material.primary
    Material.onForegroundChanged: App.setPalette(Material.foreground, Material.background)

    Component.onCompleted: {
        // propagate palette of Qt Quick Controls 2 style to regular QPalette of QGuiApplication for icon rendering
        App.setPalette(Material.foreground, Material.background);
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.ForwardButton | Qt.BackButton
        propagateComposedEvents: true
        onClicked: (event) => {
            const button = event.button;
            if (button === Qt.BackButton) {
                pageStack.pop();
            } else if (button === Qt.ForwardButton) {
                pageStack.forward();
            }
        }
    }

    readonly property string title: qsTr("Syncthing")
    readonly property var font: App.font
    required property var window
    required property var pageStack
    required property var drawer
    readonly property int spacing: 7
    readonly property CustomDialog discardChangesDialog: CustomDialog {
        id: discardChangesDialog
        title: main.title
        contentItem: Label {
            Layout.fillWidth: true
            text: qsTr("Do you really want to go back without applying changes?")
            wrapMode: Text.WordWrap
        }
        onAccepted: pageStack.pop(true)
    }
    property bool forceClose: false

    // show notifications
    ToolTip {
        anchors.centerIn: Overlay.overlay
        id: notifictionToolTip
        timeout: 5000
    }
    Connections {
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
        function onInternalErrorsRequested() {
            statusButton.showInternalErrors();
        }
        function onConnectionErrorsRequested() {
            statusButton.showConnectionErrors();
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
    Connections {
        target: App.connection
        function onNewConfigTriggered() {
            showNotifiction(qsTr("Configuration changed"));
        }
    }
    Connections {
        target: App.notifier
        function onDisconnected() {
            showNotifiction(qsTr("UI disconnected from Syncthing backend"));
        }
    }
    function showNotifiction(message) {
        if (App.showToast(message)) {
            return;
        }
        notifictionToolTip.text = notifictionToolTip.visible ? `${notifictionToolTip.text}\n${message}` : message;
        notifictionToolTip.open();
    }
}
