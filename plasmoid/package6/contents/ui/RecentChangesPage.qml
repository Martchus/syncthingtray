import QtQuick 2.3
import QtQuick.Layouts 1.1
import QtQml.Models 2.2
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kirigami 2.20 as Kirigami

Item {
    property alias view: recentChangesView
    objectName: "RecentChangesPage"

    PlasmaComponents3.ScrollView {
        anchors.fill: parent

        // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
        PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

        contentItem: TopLevelView {
            id: recentChangesView
            model: plasmoid.recentChangesModel
            delegate: TopLevelItem {
                width: recentChangesView.effectiveWidth()
                ColumnLayout {
                    width: parent.width
                    spacing: 0
                    RowLayout {
                        Layout.fillWidth: true
                        Kirigami.Icon {
                            Layout.preferredWidth: Kirigami.Units.iconSizes.small
                            Layout.preferredHeight: Kirigami.Units.iconSizes.small
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
                            width: Kirigami.Units.smallSpacing
                        }
                        Image {
                            Layout.preferredWidth: Kirigami.Units.iconSizes.small
                            Layout.preferredHeight: Kirigami.Units.iconSizes.small
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            height: parent.height
                            fillMode: Image.PreserveAspectFit
                            source: plasmoid.faUrl + "calendar"
                        }
                        PlasmaComponents3.Label {
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            elide: Text.ElideRight
                            text: eventTime
                        }
                        Item {
                            width: Kirigami.Units.smallSpacing
                        }
                        Image {
                            Layout.preferredWidth: Kirigami.Units.iconSizes.small
                            Layout.preferredHeight: Kirigami.Units.iconSizes.small
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            height: parent.height
                            fillMode: Image.PreserveAspectFit
                            source: plasmoid.faUrl + "qrcode"
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
                            Layout.preferredWidth: Kirigami.Units.iconSizes.small
                            Layout.preferredHeight: Kirigami.Units.iconSizes.small
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            height: parent.height
                            fillMode: Image.PreserveAspectFit
                            source: plasmoid.faUrl + (itemType === "file" ? "file-o" : "folder-o")
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

                readonly property string fileAction: action
                function openPath() {
                    plasmoid.openLocalFileOrDir(directoryId, path)
                }
                function copyPath() {
                    plasmoid.copyToClipboard(directoryId, path)
                }
                function copyDeviceId() {
                    plasmoid.copyToClipboard(modifiedBy)
                }
                function copyDirectoryId() {
                    plasmoid.copyToClipboard(directoryId)
                }
            }

            PlasmaExtras.Menu {
                id: contextMenu

                function init(item) {
                    openItem.enabled = item.fileAction !== "deleted";
                }

                PlasmaExtras.MenuItem {
                    id: openItem
                    text: qsTr("Open item")
                    icon: "document-open"
                    onClicked: recentChangesView.currentItem.openPath()
                }
                PlasmaExtras.MenuItem {
                    text: qsTr("Copy path")
                    icon: "edit-copy"
                    onClicked: recentChangesView.currentItem.copyPath()
                }
                PlasmaExtras.MenuItem {
                    text: qsTr("Copy device ID")
                    icon: "network-server-symbolic"
                    onClicked: recentChangesView.currentItem.copyDeviceId()
                }
                PlasmaExtras.MenuItem {
                    text: qsTr("Copy folder ID")
                    icon: "folder"
                    onClicked: recentChangesView.currentItem.copyDirectoryId()
                }
            }
        }
    }
}
