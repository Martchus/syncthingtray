import QtQuick 2.3
import QtQuick.Layouts 1.1
import QtQml.Models 2.2
import org.kde.plasma.components 2.0 as PlasmaComponents  // for Menu and MenuItem
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.0 as PlasmaCore

Item {
    property alias view: recentChangesView
    objectName: "RecentChangesPage"

    PlasmaComponents3.ScrollView {
        anchors.fill: parent

        // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
        PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

        contentItem: TopLevelView {
            id: recentChangesView
            model: plasmoid.nativeInterface.recentChangesModel
            delegate: TopLevelItem {
                width: recentChangesView.effectiveWidth()
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
                            source: plasmoid.nativeInterface.faUrl + "calendar"
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
                            source: plasmoid.nativeInterface.faUrl + "qrcode"
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
                            source: plasmoid.nativeInterface.faUrl + (itemType === "file" ? "file-o" : "folder-o")
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

                function openPath() {
                    plasmoid.nativeInterface.openLocalFileOrDir(directoryId, path)
                }
                function copyPath() {
                    plasmoid.nativeInterface.copyToClipboard(directoryId, path)
                }
                function copyDeviceId() {
                    plasmoid.nativeInterface.copyToClipboard(modifiedBy)
                }
                function copyDirectoryId() {
                    plasmoid.nativeInterface.copyToClipboard(directoryId)
                }
            }

            PlasmaComponents.Menu {
                id: contextMenu
                PlasmaComponents.MenuItem {
                    text: qsTr("Open item")
                    icon: "document-open"
                    onClicked: recentChangesView.currentItem.openPath()
                }
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
                PlasmaComponents.MenuItem {
                    text: qsTr("Copy folder ID")
                    icon: "folder"
                    onClicked: recentChangesView.currentItem.copyDirectoryId()
                }
            }
        }
    }
}
