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
        // assign controls where needed
        pageStack.searchTextArea = searchTextArea;
        drawer.aboutDialog = aboutDialog;
        drawer.closeDialog = closeDialog;

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
    readonly property ToolBar toolBar: ToolBar {
        Material.theme: Material.Dark
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: pageStack.anchors.leftMargin
            RowLayout {
                visible: App.status.length !== 0
                CustomToolButton {
                    id: statusButton
                    visible: !busyIndicator.running
                    icon.source: App.faUrlBase + "exclamation-triangle"
                    text: App.hasInternalErrors || App.connection.hasErrors ? qsTr("Show notifications/errors") : qsTr("Syncthing backend status is problematic")
                    onClicked: {
                        const hasInternalErrors = App.hasInternalErrors;
                        const hasConnectionErrors = App.connection.hasErrors;
                        if (hasInternalErrors && hasConnectionErrors) {
                            statusButtonMenu.popup(statusButton, statusButton.width / 2, statusButton.height / 2);
                        } else if (hasInternalErrors) {
                            statusButton.showInternalErrors();
                        } else if (hasConnectionErrors) {
                            statusButton.showConnectionErrors();
                        } else {
                            App.performHapticFeedback();
                        }
                    }
                    Menu {
                        id: statusButtonMenu
                        popupType: App.nativePopups ? Popup.Native : Popup.Item
                        MenuItem {
                            text: qsTr("Show API errors")
                            onTriggered: statusButton.showInternalErrors()
                        }
                        MenuItem {
                            text: qsTr("Show Syncthing errors/notifications")
                            onTriggered: statusButton.showConnectionErrors()
                        }
                    }
                    function showInternalErrors() {
                        pageStack.setCurrentIndex(5);
                        if (!(settingsPage.currentItem instanceof InternalErrorsPage)) {
                            settingsPage.push("InternalErrorsPage.qml", {}, StackView.PushTransition);
                        }
                    }
                    function showConnectionErrors() {
                        pageStack.setCurrentIndex(5);
                        if (!(settingsPage.currentItem instanceof ErrorsPage)) {
                            settingsPage.push("ErrorsPage.qml", {}, StackView.PushTransition);
                        }
                    }
                }
                BusyIndicator {
                    id: busyIndicator
                    running: App.connection.connecting || App.launcher.starting || App.savingConfig || App.importExportOngoing
                    visible: running
                    Layout.preferredWidth: statusButton.width - 5
                    Layout.preferredHeight: statusButton.height - 5
                }
                Label {
                    text: App.status
                    elide: Label.ElideRight
                    horizontalAlignment: Qt.AlignLeft
                    verticalAlignment: Qt.AlignVCenter
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                CustomToolButton {
                    visible: !App.connection.connected
                    icon.source: App.faUrlBase + "refresh"
                    text: qsTr("Try to re-connect")
                    onClicked: App.connection.connect()
                }
            }
            StackLayout {
                id: toolBarStack
                currentIndex: toolBarStack.searchAvailable && (searchTextArea.text.length > 0 || searchTextArea.activeFocus) ? 1 : 0
                RowLayout {
                    CustomToolButton {
                        visible: !backButton.visible
                        icon.source: App.faUrlBase + "bars"
                        text: qsTr("Toggle menu")
                        onClicked: drawer.visible ? drawer.close() : drawer.open()
                    }
                    CustomToolButton {
                        id: backButton
                        visible: pageStack.currentDepth > 1
                        icon.source: App.faUrlBase + "chevron-left"
                        text: qsTr("Back")
                        onClicked: pageStack.pop()
                    }
                    Label {
                        text: pageStack.currentPage.title
                        elide: Label.ElideRight
                        horizontalAlignment: Qt.AlignLeft
                        verticalAlignment: Qt.AlignVCenter
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    CustomToolButton {
                        visible: toolBarStack.searchAvailable
                        icon.source: App.faUrlBase + "search"
                        text: qsTr("Search")
                        onClicked: searchTextArea.focus = true
                    }
                    Repeater {
                        model: pageStack.currentActions
                        DelegateChooser {
                            role: "enabled"
                            DelegateChoice {
                                roleValue: true
                                CustomToolButton {
                                    required property Action modelData
                                    enabled: modelData.enabled
                                    text: modelData.text
                                    icon.source: modelData.icon.source
                                    onClicked: modelData.trigger()
                                }
                            }
                        }
                    }
                    CustomToolButton {
                        id: extraActionsMenuButton
                        visible: pageStack.currentExtraActions.length > 0
                        icon.source: App.faUrlBase + "ellipsis-v"
                        text: qsTr("More")
                        onClicked: pageStack.currentPage?.showExtraActions() ?? extraActionsMenu.popup(extraActionsMenuButton, extraActionsMenuButton.width / 2 - extraActionsMenu.width, extraActionsMenuButton.height / 2)
                        Menu {
                            id: extraActionsMenu
                            popupType: App.nativePopups ? Popup.Native : Popup.Item
                            MenuItemInstantiator {
                                menu: extraActionsMenu
                                model: {
                                    const currentPage = pageStack.currentPage;
                                    const extraActions = currentPage.showExtraActions === undefined ? currentPage.extraActions : undefined;
                                    return extraActions ?? [];
                                }
                            }
                        }
                    }
                }
                RowLayout {
                    TextArea {
                        id: searchTextArea
                        Layout.fillWidth: true
                        Layout.preferredHeight: clearSearchButton.height
                        leftInset: 2
                        topInset: 7
                        bottomInset: 2
                        topPadding: 14
                        bottomPadding: 7
                        placeholderText: qsTr("Searching %1").arg(pageStack.currentPage.title)
                        onTextChanged: {
                            if (pageStack.changingIndex) {
                                return;
                            }
                            const model = pageStack?.currentPage?.model;
                            if (model?.filterRegularExpression !== undefined) {
                                model.setFilterRegularExpressionPattern(searchTextArea.text);
                            }
                        }
                    }
                    CustomToolButton {
                        id: clearSearchButton
                        icon.source: App.faUrlBase + "times-circle-o"
                        text: qsTr("Clear search")
                        onClicked: searchTextArea.clear()
                    }
                }
                readonly property bool searchAvailable: pageStack?.currentPage?.model?.filterRegularExpressionPattern !== undefined
            }
        }
    }
    readonly property AboutDialog aboutDialog: AboutDialog {
        id: aboutDialog
        Material.primary: Material.LightBlue
        Material.accent: Material.LightBlue
    }
    readonly property TabBar tabBar: TabBar {
        visible: drawer.interactive
        currentIndex: Math.min(pageStack.currentIndex, 4)
        MainTabButton {
            text: qsTr("Start")
            iconName: "home"
            tabIndex: 0
        }
        MainTabButton {
            text: qsTr("Folders")
            iconName: "folder"
            tabIndex: 1
        }
        MainTabButton {
            text: qsTr("Devices")
            iconName: "sitemap"
            tabIndex: 2
        }
        MainTabButton {
            text: qsTr("Recent changes")
            iconName: "history"
            tabIndex: 3
        }
        MainTabButton {
            text: qsTr("More")
            iconName: "cog"
            tabIndex: 5
        }
    }
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
    readonly property Dialog closeDialog: Dialog {
        id: closeDialog
        parent: Overlay.overlay
        anchors.centerIn: Overlay.overlay
        popupType: App.nativePopups ? Popup.Native : Popup.Item
        width: Math.min(popupType === Popup.Item ? parent.width - 20 : implicitWidth, 800)
        standardButtons: Dialog.NoButton
        modal: true
        title: main.title
        contentItem: Label {
            Layout.fillWidth: true
            text: qsTr("Do you want to shutdown Syncthing and quit the app? You can also just quit the app and keep Syncthing running in the backround.")
            wrapMode: Text.WordWrap
        }
        onAccepted: {
            main.forceClose = true;
            window.close();
        }
        footer: DialogButtonBox {
            Button {
                text: qsTr("Cancel")
                flat: true
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
            Button {
                text: qsTr("Shutdown")
                flat: true
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
            Button {
                text: qsTr("Background")
                flat: true
                onClicked: {
                    closeDialog.close();
                    App.minimize();
                }
                DialogButtonBox.buttonRole: DialogButtonBox.InvalidRole
            }
        }
    }

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
