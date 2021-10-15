import QtQuick 2.3
import QtQuick.Layouts 1.1
import QtQml.Models 2.2
import org.kde.plasma.components 2.0 as PlasmaComponents  // for Menu and MenuItem
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    property alias view: recentChangesView
    anchors.fill: parent
    objectName: "RecentChangesPage"

    PlasmaExtras.ScrollArea {
        anchors.fill: parent

        TopLevelView {
            id: recentChangesView
            width: parent.width
            model: plasmoid.nativeInterface.recentChangesModel
            delegate: TopLevelItem {
                width: recentChangesView.width
                ColumnLayout {
                    width: parent.width
                    spacing: 0
                    RowLayout {
                        Layout.fillWidth: true
                        PlasmaCore.IconItem {
                            Layout.preferredWidth: units.iconSizes.small
                            Layout.preferredHeight: units.iconSizes.small
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            source: actionIcon
                        }
                        PlasmaComponents3.Label {
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            text: extendedAction
                        }
                        Item {
                            width: units.smallSpacing
                        }
                        Image {
                            Layout.preferredWidth: units.iconSizes.small
                            Layout.preferredHeight: units.iconSizes.small
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            height: parent.height
                            fillMode: Image.PreserveAspectFit
                            source: "image://fa/calendar"
                        }
                        PlasmaComponents3.Label {
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            elide: Text.ElideRight
                            text: eventTime
                        }
                        Item {
                            width: units.smallSpacing
                        }
                        Image {
                            Layout.preferredWidth: units.iconSizes.small
                            Layout.preferredHeight: units.iconSizes.small
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            height: parent.height
                            fillMode: Image.PreserveAspectFit
                            source: "image://fa/qrcode"
                        }
                        PlasmaComponents3.Label {
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            elide: Text.ElideRight
                            text: modifiedBy
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Image {
                            Layout.preferredWidth: units.iconSizes.small
                            Layout.preferredHeight: units.iconSizes.small
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            height: parent.height
                            fillMode: Image.PreserveAspectFit
                            source: itemType === "file" ? "image://fa/file-o" : "image://fa/folder-o"
                        }
                        PlasmaComponents3.Label {
                            text: directoryId + ": "
                            font.weight: Font.DemiBold
                        }
                        PlasmaComponents3.Label {
                            Layout.fillWidth: true
                            text: path
                            elide: Text.ElideRight
                        }
                    }
                }

                function copyPath() {
                    plasmoid.nativeInterface.copyToClipboard(path)
                }
                function copyDeviceId() {
                    plasmoid.nativeInterface.copyToClipboard(modifiedBy)
                }
            }

            PlasmaComponents.Menu {
                id: contextMenu
                PlasmaComponents.MenuItem {
                    text: qsTr("Copy path")
                    icon: "edit-copy"
                    onClicked: recentChangesView.currentItem.copyPath()
                }
                PlasmaComponents.MenuItem {
                    text: qsTr("Copy device ID")
                    icon: "network-server-symbolic"
                    onClicked: recentChangesView.currentItem.copyDeviceId()
                }
            }
        }
    }
}
