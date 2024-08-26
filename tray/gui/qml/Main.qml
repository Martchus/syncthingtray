import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ApplicationWindow {
    id: window
    visible: true
    width: 700
    height: 500
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: drawer.effectiveWidth
            ToolButton {
                visible: pageStack.currentDepth > 1
                text: "‹"
                onClicked: pageStack.pop()
                ToolTip.text: qsTr("Back")
                ToolTip.visible: hovered || pressed
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            }
            Label {
                text: pageStack.currentPage.title
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
            }
            ToolButton {
                icon.source: app.faUrlBase + "bars"
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
            footer: ItemDelegate {
                width: parent.width
                text: Qt.application.version
                icon.source: app.faUrlBase + "info-circle"
                onClicked: aboutDialog.visible = true
            }
            footerPositioning: ListView.OverlayFooter
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
                    name: qsTr("Syncthing")
                    iconName: "syncthing"
                }
                ListElement {
                    name: qsTr("App settings")
                    iconName: "cog"
                }
            }
            delegate: ItemDelegate {
                text: name
                icon.source: app.faUrlBase + iconName
                width: parent.width
                onClicked: {
                    drawerListView.currentIndex = index
                    drawer.position = drawer.initialPosition
                }
            }
            ScrollIndicator.vertical: ScrollIndicator { }
        }
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        anchors.leftMargin: drawer.effectiveWidth

        StackLayout {
            id: pageStack
            anchors.fill: parent
            currentIndex: drawerListView.currentIndex
            DirsPage {
            }
            DevsPage {
            }
            ChangesPage {
            }
            WebViewPage {
            }
            SettingsPage {
            }

            readonly property var currentPage: {
                const currentChild = children[currentIndex];
                return currentChild.currentItem ?? currentChild;
            }
            readonly property var currentDepth: children[currentIndex]?.depth ?? 1
            function pop() { children[currentIndex].pop?.() }
        }
    }
}
