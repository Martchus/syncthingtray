import QtQuick 2.3
import QtQuick.Layouts 1.1
import QtQml.Models 2.2
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import martchus.syncthingplasmoid 0.6 as SyncthingPlasmoid

ColumnLayout {
    property alias view: directoryView
    property alias filter: filter
    anchors.fill: parent
    objectName: "DirectoriesPage"

    PlasmaExtras.ScrollArea {
        Layout.fillWidth: true
        Layout.fillHeight: true

        TopLevelView {
            id: directoryView

            model: PlasmaCore.SortFilterModel {
                id: directoryFilterModel
                sourceModel: plasmoid.nativeInterface.dirModel
                filterRole: "name"
                filterRegExp: filter.text
            }

            delegate: TopLevelItem {
                id: item
                property alias errorsButton: errorsButton
                property alias rescanButton: rescanButton
                property alias resumePauseButton: resumePauseButton
                property alias openButton: openButton
                property int sourceIndex: directoryFilterModel.mapRowToSource(
                                              index)

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
                        PlasmaComponents.Label {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            elide: Text.ElideRight
                            text: name
                        }
                        RowLayout {
                            id: toolButtonsLayout
                            spacing: 0

                            PlasmaComponents.Label {
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
                                iconSource: ":/icons/hicolor/scalable/emblems/emblem-important-old.svg"
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
                                iconSource: "view-refresh"
                                tooltip: qsTr("Rescan")
                                enabled: !paused
                                onClicked: plasmoid.nativeInterface.connection.rescan(
                                               dirId)
                            }
                            TinyButton {
                                id: resumePauseButton
                                iconSource: paused ? "media-playback-start" : "media-playback-pause"
                                tooltip: paused ? qsTr("Resume") : qsTr("Pause")
                                onClicked: {
                                    paused ? plasmoid.nativeInterface.connection.resumeDirectories(
                                                 [dirId]) : plasmoid.nativeInterface.connection.pauseDirectories(
                                                 [dirId])
                                }
                            }
                            TinyButton {
                                id: openButton
                                iconSource: "folder"
                                tooltip: qsTr("Open in file browser")
                                onClicked: {
                                    Qt.openUrlExternally(path)
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
                            rootIndex: detailsView.model.modelIndex(sourceIndex)
                            delegate: DetailItem {
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
                    resumePauseItem.icon = item.resumePauseButton.iconSource
                }

                PlasmaComponents.MenuItem {
                    id: rescanItem
                    text: qsTr('Rescan')
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
                    text: qsTr('Open in file browser')
                    icon: "folder"
                    onClicked: directoryView.clickCurrentItemButton(
                                   "openButton")
                }
            }
        }
    }

    PlasmaComponents.TextField {
        property bool explicitelyShown: false
        id: filter
        clearButtonShown: true
        Layout.fillWidth: true
        visible: explicitelyShown || text !== ""
    }
}
