import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main
import Tray

Pane {
    id: mainPane
    padding: 0
    anchors.fill: parent
    font.pointSize: QuickUI.font.pointSize
    Material.theme: theming.Material.theme
    Material.primary: theming.Material.primary
    Material.accent: theming.Material.accent
    background: Rectangle {
        color: theming.baseColor(mainPane.palette)
    }
    ColumnLayout {
        anchors.fill: parent
        Keys.onLeftPressed: (event) => {
            tabBar.setCurrentIndex(tabBar.currentIndex > 0 ? tabBar.currentIndex - 1 : tabBar.count - 1);
            pageStack.setCurrentIndex(tabBar.currentItem.tabIndex);
        }
        Keys.onRightPressed: (event) => {
            tabBar.setCurrentIndex(tabBar.currentIndex < tabBar.count - 1 ? tabBar.currentIndex + 1 : 0);
            pageStack.setCurrentIndex(tabBar.currentItem.tabIndex);
        }
        Keys.onPressed: (event) => {
            if ((event.modifiers === Qt.NoModifier) && ((event.key >= Qt.Key_0 && event.key <= Qt.Key_9) || (event.key >= Qt.Key_A && event.key <= Qt.Key_Z)) && searchField.enabled) {
                searchField.visible = true;
                searchField.text += event.text;
                searchField.forceActiveFocus();
            }
        }
        StackLayout {
            id: pageStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            onCurrentIndexChanged: {
                TrayWidget.handleCurrentTabChanged(pageStack.currentIndex);
                searchField.init(pageStack.currentIndex);
            }
            DirListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                anchors.fill: null
                focus: StackLayout.isCurrentItem
                stackView: null
            }
            DevListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                anchors.fill: null
                focus: StackLayout.isCurrentItem
                stackView: null
            }
            ChangesListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                focus: StackLayout.isCurrentItem
                anchors.fill: null
            }
            readonly property Item currentItem: pageStack.itemAt(pageStack.currentIndex)
            function setCurrentIndex(index) {
                pageStack.currentIndex = Math.min(Math.max(0, index), pageStack.count - 1);
            }
        }
        Pane {
            padding: 0
            Layout.fillWidth: true
            contentWidth: tabBar.implicitWidth
            contentHeight: tabBar.implicitHeight
            TabBar {
                id: tabBar
                anchors.fill: parent
                topPadding: 2
                bottomPadding: 2
                rightPadding: toolBar.width
                position: TabBar.Footer
                MainTabButton {
                    id: foldersButton
                    text: qsTr("Folders")
                    iconName: "folder"
                    tabIndex: 0
                    displayWithIcon: AbstractButton.TextBesideIcon
                }
                MainTabButton {
                    id: devsButton
                    text: qsTr("Devices")
                    iconName: "sitemap"
                    tabIndex: 1
                    displayWithIcon: AbstractButton.TextBesideIcon
                }
                MainTabButton {
                    text: qsTr("Recent changes")
                    iconName: "history"
                    tabIndex: 2
                    displayWithIcon: AbstractButton.TextBesideIcon
                }
            }
            RowLayout{
                id: toolBar
                anchors.right: tabBar.right
                anchors.verticalCenter: tabBar.verticalCenter
                IconOnlyButton {
                    visible: tabBar.currentIndex === foldersButton.tabIndex
                    text: qsTr("Add folder")
                    icon.source: QuickUI.faUrlBase + "plus"
                    onClicked: QuickUI.editDir("", "", null)
                }
                IconOnlyButton {
                    visible: tabBar.currentIndex === devsButton.tabIndex
                    text: qsTr("Add device")
                    icon.source: QuickUI.faUrlBase + "plus"
                    onClicked: QuickUI.editDev("", "", null)
                }
                IconOnlyButton {
                    text: qsTr("Rescan all")
                    icon.source: QuickUI.faUrlBase + "refresh"
                    onClicked: SyncthingData.connection.rescanAllDirs()
                }
                IconOnlyButton {
                    text: qsTr("Show ID")
                    icon.source: QuickUI.faUrlBase + "qrcode"
                    onClicked: TrayWidget.showOwnDeviceId()
                }
                IconOnlyButton {
                    text: qsTr("Show logs")
                    icon.source: QuickUI.faUrlBase + "terminal"
                    onClicked: TrayWidget.showLog()
                }
                IconOnlyButton {
                    text: qsTr("Filter")
                    icon.source: QuickUI.faUrlBase + "search"
                    onClicked: searchField.toggle()
                }
            }
            SearchField {
                id: searchField
                visible: false
                focus: visible
                anchors.bottom: toolBar.top
                anchors.right: toolBar.right
                width: 250
                live: true
                enabled: pageStack.currentItem.mainModel !== undefined
                onSearchTriggered: searchField.apply(searchField.text)
                onClearButtonPressed: searchField.apply("", true)
                Keys.onEscapePressed: searchField.apply(searchField.text = "", true)
                Component.onCompleted: searchField.init();
                function toggle() {
                    return ((searchField.visible = !searchField.visible)) && searchField.forceActiveFocus();
                }
                function init(index) {
                    searchField.text = pageStack.itemAt(index ?? pageStack.currentIndex).mainModel?.filterRegularExpressionPattern() ?? "";
                }
                function apply(text, hide) {
                    pageStack.currentItem.mainModel?.setFilterRegularExpressionPattern(text);
                    if (hide) {
                        searchField.visible = false;
                    }
                }
            }
        }
    }
    readonly property Theming theming: Theming {
        currentPage: null
    }
}
