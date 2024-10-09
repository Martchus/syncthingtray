import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
//import QtQuick.Controls.Material
import QtQuick.Dialogs
import Qt.labs.qmlmodels

ApplicationWindow {
    id: window
    visible: true
    width: 700
    height: 500
    title: qsTr("Syncthing App")
    onVisibleChanged: app.setCurrentControls(window.visible, pageStack.currentIndex)
    //Material.theme: app.darkmodeEnabled ? Material.Dark : Material.Light
    header: ToolBar {
        //Material.theme: Material.Dark
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: flickable.anchors.leftMargin
            RowLayout {
                visible: app.status.length !== 0
                ToolButton {
                    id: statusButton
                    visible: !busyIndicator.running
                    icon.source: app.faUrlBase + "exclamation-triangle"
                    icon.width: app.iconSize
                    icon.height: app.iconSize
                    text: qsTr("Syncthing backend status is problematic")
                    display: AbstractButton.IconOnly
                    onPressAndHold: app.performHapticFeedback()
                    ToolTip.text: text
                    ToolTip.visible: hovered || pressed
                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                }
                BusyIndicator {
                    id: busyIndicator
                    running: app.connection.connecting
                    visible: running
                    Layout.preferredWidth: statusButton.width - 5
                    Layout.preferredHeight: statusButton.height - 5
                }
                Label {
                    text: app.status
                    elide: Label.ElideRight
                    horizontalAlignment: Qt.AlignLeft
                    verticalAlignment: Qt.AlignVCenter
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                ToolButton {
                    visible: !app.connection.connected
                    icon.source: app.faUrlBase + "refresh"
                    icon.width: app.iconSize
                    icon.height: app.iconSize
                    text: qsTr("Try to re-connect")
                    display: AbstractButton.IconOnly
                    onClicked: app.connection.connect()
                    onPressAndHold: app.performHapticFeedback()
                    ToolTip.text: text
                    ToolTip.visible: hovered || pressed
                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                }
            }
            RowLayout {
                ToolButton {
                    visible: !backButton.visible
                    icon.source: app.faUrlBase + "bars"
                    icon.width: app.iconSize
                    icon.height: app.iconSize
                    text: qsTr("Toggle menu")
                    display: AbstractButton.IconOnly
                    onClicked: drawer.visible ? drawer.close() : drawer.open()
                    onPressAndHold: app.performHapticFeedback()
                    ToolTip.text: text
                    ToolTip.visible: hovered || pressed
                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                }
                ToolButton {
                    id: backButton
                    visible: pageStack.currentDepth > 1
                    icon.source: app.faUrlBase + "chevron-left"
                    icon.width: app.iconSize
                    icon.height: app.iconSize
                    text: qsTr("Back")
                    display: AbstractButton.IconOnly
                    onClicked: pageStack.pop()
                    onPressAndHold: app.performHapticFeedback()
                    ToolTip.text: text
                    ToolTip.visible: hovered || pressed
                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                }
                Label {
                    text: pageStack.currentPage.title
                    elide: Label.ElideRight
                    horizontalAlignment: Qt.AlignLeft
                    verticalAlignment: Qt.AlignVCenter
                    Layout.fillWidth: true
                }
                Repeater {
                    model: pageStack.currentActions
                    DelegateChooser {
                        role: "enabled"
                        DelegateChoice {
                            roleValue: true
                            ToolButton {
                                required property Action modelData
                                enabled: modelData.enabled
                                text: modelData.text
                                display: AbstractButton.IconOnly
                                ToolTip.visible: hovered || pressed
                                ToolTip.text: modelData.text
                                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                                icon.source: modelData.icon.source
                                icon.width: app.iconSize
                                icon.height: app.iconSize
                                onClicked: modelData.trigger()
                                onPressAndHold: app.performHapticFeedback()
                            }
                        }
                    }
                }
                ToolButton {
                    visible: pageStack.currentExtraActions.length > 0
                    icon.source: app.faUrlBase + "ellipsis-v"
                    icon.width: app.iconSize
                    icon.height: app.iconSize
                    onClicked: extraActionsMenu.popup()
                    onPressAndHold: app.performHapticFeedback()
                }
                Menu {
                    id: extraActionsMenu
                    Instantiator {
                        model: pageStack.currentExtraActions
                        delegate: MenuItem {
                            required property Action modelData
                            text: modelData.text
                            enabled: modelData.enabled
                            icon.source: modelData.icon.source
                            icon.width: app.iconSize
                            icon.height: app.iconSize
                            onTriggered: modelData?.trigger()
                        }
                        onObjectAdded: (index, object) => object.enabled && extraActionsMenu.insertItem(index, object)
                        onObjectRemoved: (index, object) => extraActionsMenu.removeItem(object)
                    }
                }
            }
        }
    }

    readonly property bool inPortrait: window.width < window.height
    readonly property int spacing: 7

    AboutDialog {
        id: aboutDialog
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

        ListView {
            id: drawerListView
            anchors.fill: parent
            keyNavigationEnabled: true
            footer: ItemDelegate {
                width: parent.width
                text: Qt.application.version
                icon.source: app.faUrlBase + "info-circle"
                icon.width: app.iconSize
                icon.height: app.iconSize
                onClicked: aboutDialog.visible = true
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
                icon.source: app.faUrlBase + iconName
                icon.width: app.iconSize
                icon.height: app.iconSize
                width: parent.width
                onClicked: {
                    pageStack.setCurrentIndex(index);
                    drawer.position = drawer.initialPosition;
                }
            }
            ScrollIndicator.vertical: ScrollIndicator { }
        }
    }
    footer: TabBar {
        visible: drawer.interactive
        currentIndex: Math.min(pageStack.currentIndex, 3)
        TabButton {
            text: qsTr("Folders")
            display: AbstractButton.IconOnly
            icon.source: app.faUrlBase + "folder"
            icon.width: app.iconSize
            icon.height: app.iconSize
            onClicked: pageStack.setCurrentIndex(0)
            onPressAndHold: app.performHapticFeedback()
            ToolTip.visible: hovered || pressed
            ToolTip.text: text
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        }
        TabButton {
            text: qsTr("Devices")
            display: AbstractButton.IconOnly
            icon.source: app.faUrlBase + "sitemap"
            icon.width: app.iconSize
            icon.height: app.iconSize
            onClicked: pageStack.setCurrentIndex(1)
            onPressAndHold: app.performHapticFeedback()
            ToolTip.visible: hovered || pressed
            ToolTip.text: text
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        }
        TabButton {
            text: qsTr("Recent changes")
            display: AbstractButton.IconOnly
            icon.source: app.faUrlBase + "history"
            icon.width: app.iconSize
            icon.height: app.iconSize
            onClicked: pageStack.setCurrentIndex(2)
            onPressAndHold: app.performHapticFeedback()
            ToolTip.visible: hovered || pressed
            ToolTip.text: text
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        }
        TabButton {
            text: qsTr("More")
            display: AbstractButton.IconOnly
            icon.source: app.faUrlBase + "cog"
            icon.width: app.iconSize
            icon.height: app.iconSize
            onClicked: pageStack.setCurrentIndex(5)
            onPressAndHold: app.performHapticFeedback()
            ToolTip.visible: hovered || pressed
            ToolTip.text: text
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        }
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        anchors.leftMargin: drawer.visible ? drawer.effectiveWidth : 0

        SwipeView {
            id: pageStack
            anchors.fill: parent
            currentIndex: 0
            onCurrentIndexChanged: {
                indexHistory.push(pageStack.currentIndex)
                app.setCurrentControls(window.visible, pageStack.currentIndex)
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
            function pop() {
                const currentChild = children[currentIndex];
                const currentPage = currentChild.currentItem ?? currentChild;
                return currentPage.back?.() || currentChild.pop?.() || pageStack.back();
            }
            readonly property var indexHistory: []
            function back() {
                if (indexHistory.length < 1) {
                    return false;
                }
                const currentIndex = indexHistory.pop();
                const previousIndex = indexHistory.pop();
                pageStack.setCurrentIndex(previousIndex);
                return true;
            }
        }
    }

    // handle keyboard events
    Component.onCompleted: {
        window.contentItem.forceActiveFocus(Qt.ActiveWindowFocusReason);
        window.contentItem.Keys.released.connect((event) => {
            console.log("main key event: 0x" + event.key.toString(16))
            if (event.key === Qt.Key_Back || (event.key === Qt.Key_Backspace && typeof activeFocusItem.getText !== "function")) {
                event.accepted = pageStack.pop();
            } else if (event.key === Qt.Key_F5) {
                event.accepted = app.reload();
            }
        });
    }
    onActiveFocusItemChanged: {
        console.log("focus item: " + activeFocusItem?.toString())
        if (activeFocusItem?.toString().startsWith("QQuickPopupItem")) {
            window.contentItem.forceActiveFocus(Qt.ActiveWindowFocusReason);
        }
    }

    // avoid closing app (TODO: allow to keep Syncthing running in the background)
    MessageDialog {
        id: closeDialog
        buttons: MessageDialog.Yes | MessageDialog.No
        title: window.title
        text: qsTr("Do you really want to close Syncthing?")
        onAccepted: {
            window.forceClose = true;
            window.close();
        }
    }
    onClosing: (event) => {
        if (!window.forceClose) {
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
        target: app
        function onError(message) {
            showNotifiction(message);
        }
        function onInfo(message) {
            showNotifiction(message);
        }
    }
    Connections {
        target: app.connection
        function onError(message) {
            showNotifiction(message);
        }
        function onNewConfigTriggered() {
            showNotifiction(qsTr("Configuration changed"));
        }
    }
    Connections {
        target: app.notifier
        function onDisconnected() {
            showNotifiction(qsTr("UI disconnected from Syncthing backend"));
        }
    }
    function showNotifiction(message) {
        if (app.showToast(message)) {
            return;
        }
        notifictionToolTip.text = message;
        notifictionToolTip.open();
    }
}
