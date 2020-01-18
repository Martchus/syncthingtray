import QtQuick 2.3
import QtQuick.Layouts 1.1
import QtQml.Models 2.2
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.components 2.0 as PlasmaComponents
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
            model: plasmoid.nativeInterface.recentChangesModel
            delegate: TopLevelItem {
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
                        PlasmaComponents.Label {
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            text: extendedAction
                        }
                        Item {
                            width: units.smallSpacing
                        }
                        PlasmaCore.IconItem {
                            Layout.preferredWidth: units.iconSizes.small
                            Layout.preferredHeight: units.iconSizes.small
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            source: "change-date-symbolic"
                        }
                        PlasmaComponents.Label {
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            elide: Text.ElideRight
                            text: eventTime
                        }
                        Item {
                            width: units.smallSpacing
                        }
                        PlasmaCore.IconItem {
                            Layout.preferredWidth: units.iconSizes.small
                            Layout.preferredHeight: units.iconSizes.small
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            source: "network-server-symbolic"
                        }
                        PlasmaComponents.Label {
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            elide: Text.ElideRight
                            text: modifiedBy
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        PlasmaCore.IconItem {
                            Layout.preferredWidth: units.iconSizes.small
                            Layout.preferredHeight: units.iconSizes.small
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            source: itemType === "file" ? "view-refresh-symbolic" : "folder-sync"
                        }
                        PlasmaComponents.Label {
                            text: directoryId + ": "
                            font.weight: Font.DemiBold
                        }
                        PlasmaComponents.Label {
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
