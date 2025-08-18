import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import Qt.labs.qmlmodels

import Main

ToolBar {
    id: toolBar
    Material.theme: Material.Light
    Material.background: Material.primary
    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: leftMargin
        Material.theme: Material.Dark
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
                    const settingsPage = pageStack.showPage(5);
                    if (!(settingsPage.currentItem instanceof InternalErrorsPage)) {
                        settingsPage.push("InternalErrorsPage.qml", {}, StackView.PushTransition);
                    }
                }
                function showConnectionErrors() {
                    const settingsPage = pageStack.showPage(5);
                    if (!(settingsPage.currentItem instanceof ErrorsPage)) {
                        settingsPage.push("ErrorsPage.qml", {}, StackView.PushTransition);
                    }
                }
            }
            BusyIndicator {
                id: busyIndicator
                Material.accent: Material.foreground
                running: App.connection.connecting || App.syncthingStarting || App.savingConfig || App.importExportOngoing
                visible: running
                Layout.preferredWidth: statusButton.width - Layout.margins * 2
                Layout.preferredHeight: statusButton.height - Layout.margins * 2
                Layout.margins: 5
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
                onClicked: App.connectToSyncthing()
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
                    onTextChanged: pageStack.updateSearchText(searchTextArea.text)
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
    Connections {
        target: pageStack
        function onSearchTextChanged() {
            searchTextArea.text = pageStack.searchText;
        }
    }
    Connections {
        target: App
        function onInternalErrorsRequested() {
            statusButton.showInternalErrors();
        }
        function onConnectionErrorsRequested() {
            statusButton.showConnectionErrors();
        }
    }

    required property LeftDrawer drawer
    required property PageStack pageStack
    property real leftMargin: 0
}
