import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import Qt.labs.qmlmodels

import Main

ApplicationWindow {
    id: window
    visible: true
    width: 700
    height: 500
    title: qsTr("Syncthing")
    onVisibleChanged: App.setCurrentControls(window.visible, pageStack.currentIndex)
    Material.theme: App.darkmodeEnabled ? Material.Dark : Material.Light
    Material.primary: pageStack.currentPage.isDangerous ? Material.Red : Material.LightBlue
    Material.accent: Material.primary
    header: ToolBar {
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
                            statusButtonMenu.popup();
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
                    function showErrors(page) {
                        pageStack.setCurrentIndex(5);
                        settingsPage.push(page, {}, StackView.PushTransition);
                    }
                    function showInternalErrors() {
                        statusButton.showErrors("InternalErrorsPage.qml");
                    }
                    function showConnectionErrors() {
                        statusButton.showErrors("ErrorsPage.qml");
                    }
                }
                BusyIndicator {
                    id: busyIndicator
                    running: App.connection.connecting || App.launcher.starting || App.savingConfig
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
                currentIndex: searchTextArea.text.length > 0 || searchTextArea.activeFocus ? 1 : 0
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
                        Layout.fillWidth: true
                    }
                    CustomToolButton {
                        visible: pageStack?.currentPage?.model?.filterRegularExpressionPattern !== undefined
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
                        visible: pageStack.currentExtraActions.length > 0
                        icon.source: App.faUrlBase + "ellipsis-v"
                        onClicked: extraActionsMenu.popup()
                        Menu {
                            id: extraActionsMenu
                            popupType: App.nativePopups ? Popup.Native : Popup.Item
                            Instantiator {
                                model: pageStack.currentExtraActions
                                delegate: MenuItem {
                                    required property Action modelData
                                    text: modelData.text
                                    enabled: modelData.enabled
                                    icon.source: modelData.icon.source
                                    icon.width: App.iconSize
                                    icon.height: App.iconSize
                                    onTriggered: modelData?.trigger()
                                }
                                onObjectAdded: (index, object) => object.enabled && extraActionsMenu.insertItem(index, object)
                                onObjectRemoved: (index, object) => extraActionsMenu.removeItem(object)
                            }
                        }
                    }
                }
                RowLayout {
                    TextArea {
                        id: searchTextArea
                        Layout.fillWidth: true
                        placeholderText: qsTr("Search %1").arg(pageStack.currentPage.title)
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
                        visible: pageStack?.currentPage?.model?.filterRegularExpressionPattern !== undefined
                        icon.source: App.faUrlBase + "times-circle-o"
                        onClicked: searchTextArea.clear()
                    }
                }
            }
        }
    }

    readonly property bool inPortrait: window.width < window.height
    readonly property int spacing: 7

    AboutDialog {
        id: aboutDialog
        Material.primary: Material.LightBlue
        Material.accent: Material.LightBlue
    }

    Drawer {
        id: drawer
        width: Math.min(0.66 * window.width, 200)
        height: window.height
        interactive: inPortrait || window.width < 600
        modal: interactive
        position: initialPosition
        visible: !interactive

        readonly property double initialPosition: interactive ? 0 : 1
        readonly property int effectiveWidth: !interactive ? width : 0

        CustomListView {
            id: drawerListView
            anchors.fill: parent
            footer: ColumnLayout {
                width: parent.width
                ItemDelegate {
                    Layout.fillWidth: true
                    text: Qt.application.version
                    icon.source: App.faUrlBase + "info-circle"
                    icon.width: App.iconSize
                    icon.height: App.iconSize
                    onClicked: aboutDialog.visible = true
                }
                ItemDelegate {
                    Layout.fillWidth: true
                    text: qsTr("Quit")
                    icon.source: App.faUrlBase + "power-off"
                    icon.width: App.iconSize
                    icon.height: App.iconSize
                    onClicked: closeDialog.visible = true
                }
            }
            model: ListModel {
                ListElement {
                    name: qsTr("Folders")
                    iconName: "folder"
                }
                ListElement {
                    name: qsTr("Devices")
                    iconName: "sitemap"
                }
                ListElement {
                    name: qsTr("Recent changes")
                    iconName: "history"
                }
                ListElement {
                    name: qsTr("Web-based UI")
                    iconName: "syncthing"
                }
                ListElement {
                    name: qsTr("Advanced")
                    iconName: "cogs"
                }
                ListElement {
                    name: qsTr("App settings")
                    iconName: "cog"
                }
            }
            delegate: ItemDelegate {
                id: drawerDelegate
                text: name
                activeFocusOnTab: true
                icon.source: App.faUrlBase + iconName
                icon.width: App.iconSize
                icon.height: App.iconSize
                width: parent.width
                onClicked: {
                    pageStack.setCurrentIndex(index);
                    drawer.position = drawer.initialPosition;
                }
            }
        }
    }
    footer: TabBar {
        visible: drawer.interactive
        currentIndex: Math.min(pageStack.currentIndex, 3)
        TabButton {
            text: qsTr("Folders")
            display: AbstractButton.IconOnly
            icon.source: App.faUrlBase + "folder"
            icon.width: App.iconSize
            icon.height: App.iconSize
            onClicked: pageStack.setCurrentIndex(0)
            onPressAndHold: App.performHapticFeedback()
            ToolTip.visible: hovered || pressed
            ToolTip.text: text
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        }
        TabButton {
            text: qsTr("Devices")
            display: AbstractButton.IconOnly
            icon.source: App.faUrlBase + "sitemap"
            icon.width: App.iconSize
            icon.height: App.iconSize
            onClicked: pageStack.setCurrentIndex(1)
            onPressAndHold: App.performHapticFeedback()
            ToolTip.visible: hovered || pressed
            ToolTip.text: text
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        }
        TabButton {
            text: qsTr("Recent changes")
            display: AbstractButton.IconOnly
            icon.source: App.faUrlBase + "history"
            icon.width: App.iconSize
            icon.height: App.iconSize
            onClicked: pageStack.setCurrentIndex(2)
            onPressAndHold: App.performHapticFeedback()
            ToolTip.visible: hovered || pressed
            ToolTip.text: text
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        }
        TabButton {
            text: qsTr("More")
            display: AbstractButton.IconOnly
            icon.source: App.faUrlBase + "cog"
            icon.width: App.iconSize
            icon.height: App.iconSize
            onClicked: pageStack.setCurrentIndex(5)
            onPressAndHold: App.performHapticFeedback()
            ToolTip.visible: hovered || pressed
            ToolTip.text: text
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        }
    }

    SwipeView {
        id: pageStack
        anchors.fill: parent
        anchors.leftMargin: drawer.visible ? drawer.effectiveWidth : 0
        currentIndex: 0
        onCurrentIndexChanged: {
            const newIndex = pageStack.currentIndex;
            const indexHistoryLength = indexHistory.length;
            if (indexHistoryLength > 1 && indexHistory[indexHistoryLength - 2] === newIndex) {
                indexHistory.pop();
            } else {
                indexHistory.push(newIndex);
            }
            if (!goingBackAndForth) {
                indexForward.splice(0, indexForward.length);
            }
            goingBackAndForth = false;
            App.setCurrentControls(window.visible, newIndex);

            // update search text
            pageStack.changingIndex = true;
            searchTextArea.text = pageStack.children[currentIndex]?.currentItem?.model?.filterRegularExpressionPattern();
            pageStack.changingIndex = false;
        }

        DirsPage {
            id: dirsPage
        }
        DevsPage {
            id: devsPage
        }
        ChangesPage {
            id: changesPage
        }
        WebViewPage {
            id: webViewPage
            active: pageStack.currentIndex === 3
        }
        AdvancedPage {
            id: advancedPage
        }
        SettingsPage {
            id: settingsPage
        }

        readonly property list<Item> children: [dirsPage, devsPage, changesPage, webViewPage, advancedPage, settingsPage]
        readonly property var currentPage: {
            const currentChild = children[currentIndex];
            return currentChild.currentItem ?? currentChild;
        }
        readonly property var currentDepth: children[currentIndex]?.depth ?? 1
        readonly property var currentActions: currentPage.actions ?? []
        readonly property var currentExtraActions: currentPage.extraActions ?? []
        property bool changingIndex: false
        function pop(force) {
            const currentChild = children[currentIndex];
            const currentPage = currentChild.currentItem ?? currentChild;
            if (!force && currentPage.hasUnsavedChanges) {
                const parentPage = currentPage.parentPage;
                if (parentPage?.hasUnsavedChanges !== undefined) {
                    parentPage.hasUnsavedChanges = true;
                    currentPage.hasUnsavedChanges = false;
                } else {
                    discardChangesDialog.open();
                    return true;
                }
            }
            return currentPage.back?.() || currentChild.pop?.() || pageStack.back();
        }
        readonly property var indexHistory: [0]
        readonly property var indexForward: []
        property bool goingBackAndForth: false
        function back() {
            if (indexHistory.length < 1) {
                return false;
            }
            const currentIndex = indexHistory.pop();
            const previousIndex = indexHistory.pop();
            indexForward.push(currentIndex);
            goingBackAndForth = true;
            pageStack.setCurrentIndex(previousIndex);
            return true;
        }
        function forward() {
            if (indexForward.length < 1) {
                return false;
            }
            goingBackAndForth = true;
            pageStack.setCurrentIndex(indexForward.pop());
            return true;
        }
    }
    CustomDialog {
        id: discardChangesDialog
        title: window.title
        contentItem: Label {
            Layout.fillWidth: true
            text: qsTr("Do you really want to go back without applying changes?")
            wrapMode: Text.WordWrap
        }
        onAccepted: pageStack.pop(true)
    }

    // handle global keyboard and mouse events
    Component.onCompleted: {
        window.contentItem.forceActiveFocus(Qt.ActiveWindowFocusReason);
        window.contentItem.Keys.released.connect((event) => {
            const key = event.key;
            if (key === Qt.Key_Back || (key === Qt.Key_Backspace && typeof activeFocusItem.getText !== "function")) {
                event.accepted = pageStack.pop();
            } else if (key === Qt.Key_Forward) {
                event.accepted = pageStack.forward();
            } else if (key === Qt.Key_F5) {
                event.accepted = App.reloadMain();
            }
        });
    }
    onActiveFocusItemChanged: {
        if (activeFocusItem?.toString().startsWith("QQuickPopupItem")) {
            window.contentItem.forceActiveFocus(Qt.ActiveWindowFocusReason);
        }
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

    // avoid closing app (TODO: allow to keep Syncthing running in the background)
    CustomDialog {
        id: closeDialog
        title: window.title
        contentItem: Label {
            Layout.fillWidth: true
            text: qsTr("Do you really want to close Syncthing?")
            wrapMode: Text.WordWrap
        }
        onAccepted: {
            window.forceClose = true;
            window.close();
        }
    }
    onClosing: (event) => {

        if (!window.forceClose && App.launcher.running) {
            event.accepted = false;
            closeDialog.open();
        }
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
        notifictionToolTip.text = message;
        notifictionToolTip.open();
    }
}
