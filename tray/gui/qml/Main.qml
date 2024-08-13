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
            Label {
                text: pageStack.currentPage.title
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
            }
        }
    }

    readonly property bool inPortrait: window.width < window.height
    readonly property int spacing: 7
    readonly property string faUrlBase: "image://fa/"

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
                icon.source: window.faUrlBase + "info-circle"
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
                icon.source: window.faUrlBase + iconName
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

            readonly property Page currentPage: children[currentIndex]

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
        }
    }
}
