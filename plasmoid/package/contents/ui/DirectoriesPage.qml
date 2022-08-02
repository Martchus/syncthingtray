import QtQuick 2.3
import QtQuick.Layouts 1.1
import QtQml.Models 2.2
import org.kde.plasma.components 2.0 as PlasmaComponents  // for Menu and MenuItem
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.0 as PlasmaCore
import martchus.syncthingplasmoid 0.6 as SyncthingPlasmoid

ColumnLayout {
    property alias view: directoryView
    property alias filter: filter
    objectName: "DirectoriesPage"

    PlasmaComponents3.TextField {
        property bool explicitelyShown: false
        id: filter
        clearButtonShown: true
        Layout.fillWidth: true
        visible: explicitelyShown || text !== ""
        placeholderText: qsTr("Filter directories")
        onTextChanged: directoryView.model.filterRegularExpression = new RegExp(text)
    }

    PlasmaComponents3.ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true

        // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
        PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

        contentItem: TopLevelView {
            id: directoryView
            model: plasmoid.nativeInterface.sortFilterDirModel

            delegate: TopLevelItem {
                id: item
                width: directoryView.width
                readonly property string dirName: name
                readonly property string dirPath: path
                property alias errorsButton: errorsButton
                property alias rescanButton: rescanButton
                property alias resumePauseButton: resumePauseButton
                property alias openButton: openButton

                ColumnLayout {
                    width: parent.width
                    spacing: 0

                    RowLayout {
                        id: itemSummary
                        Layout.fillWidth: true

                        PlasmaCore.IconItem {
                            Layout.preferredWidth: units.iconSizes.small
                            Layout.preferredHeight: units.iconSizes.small
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            source: statusIcon
                        }
                        PlasmaComponents3.Label {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            elide: Text.ElideRight
                            text: name
                        }
                        RowLayout {
                            id: toolButtonsLayout
                            spacing: 0

                            PlasmaComponents3.Label {
                                height: implicitHeight
                                text: statusString
                                color: statusColor ? statusColor : PlasmaCore.ColorScope.textColor
                                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            }
                            Item {
                                width: units.smallSpacing
                            }
                            TinyButton {
                                id: errorsButton
                                icon.source: plasmoid.nativeInterface.faUrl + "exclamation-triangle"
                                tooltip: qsTr("Show errors")
                                visible: pullErrorCount > 0
                                onClicked: {
                                    plasmoid.nativeInterface.showDirectoryErrors(
                                                index)
                                    plasmoid.expanded = false
                                }
                            }
                            TinyButton {
                                id: rescanButton
                                icon.source: plasmoid.nativeInterface.faUrl + "refresh"
                                tooltip: qsTr("Rescan")
                                enabled: !paused
                                onClicked: plasmoid.nativeInterface.connection.rescan(
                                               dirId)
                            }
                            TinyButton {
                                id: resumePauseButton
                                icon.source: plasmoid.nativeInterface.faUrl + (paused ? "play" : "pause")
                                tooltip: paused ? qsTr("Resume") : qsTr("Pause")
                                onClicked: {
                                    paused ? plasmoid.nativeInterface.connection.resumeDirectories(
                                                 [dirId]) : plasmoid.nativeInterface.connection.pauseDirectories(
                                                 [dirId])
                                }
                            }
                            TinyButton {
                                id: openButton
                                icon.source: plasmoid.nativeInterface.faUrl + "folder"
                                tooltip: qsTr("Open in file browser")
                                onClicked: {
                                    Qt.openUrlExternally(plasmoid.nativeInterface.substituteTilde(path))
                                    plasmoid.expanded = false
                                }
                            }
                        }
                    }

                    DetailView {
                        id: detailsView
                        visible: item.expanded
                        Layout.fillWidth: true
                        Layout.topMargin: 3

                        model: DelegateModel {
                            model: plasmoid.nativeInterface.dirModel
                            rootIndex: directoryView.model.mapToSource(directoryView.model.index(index, 0))
                            delegate: DetailItem {
                                width: detailsView.width
                            }
                        }
                    }
                }
            }

            PlasmaComponents.Menu {
                id: contextMenu

                function init(item) {
                    // use value for properties depending on paused state from buttons
                    rescanItem.enabled = item.rescanButton.enabled
                    resumePauseItem.text = item.resumePauseButton.tooltip
                    resumePauseItem.icon = item.resumePauseButton.icon
                }

                PlasmaComponents.MenuItem {
                    text: qsTr("Copy label/ID")
                    icon: "edit-copy"
                    onClicked: directoryView.copyCurrentItemData("dirName")
                }
                PlasmaComponents.MenuItem {
                    text: qsTr("Copy path")
                    icon: "edit-copy"
                    onClicked: directoryView.copyCurrentItemData("dirPath")
                }
                PlasmaComponents.MenuItem {
                    separator: true
                }
                PlasmaComponents.MenuItem {
                    id: rescanItem
                    text: qsTr("Rescan")
                    icon: "view-refresh"
                    onClicked: directoryView.clickCurrentItemButton(
                                   "rescanButton")
                }
                PlasmaComponents.MenuItem {
                    id: resumePauseItem
                    text: qsTr("Pause")
                    icon: "media-playback-pause"
                    onClicked: directoryView.clickCurrentItemButton(
                                   "resumePauseButton")
                }
                PlasmaComponents.MenuItem {
                    id: openItem
                    text: qsTr("Open in file browser")
                    icon: "folder"
                    onClicked: directoryView.clickCurrentItemButton(
                                   "openButton")
                }
            }
        }
    }
}
