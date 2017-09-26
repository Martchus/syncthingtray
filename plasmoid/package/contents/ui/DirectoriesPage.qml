import QtQuick 2.3
import QtQuick.Layouts 1.1
import QtQml.Models 2.2
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import martchus.syncthingplasmoid 0.6 as SyncthingPlasmoid

Item {
    property alias view: directoryView
    anchors.fill: parent
    objectName: "DirectoriesPage"

    PlasmaExtras.ScrollArea {
        anchors.fill: parent

        TopLevelView {
            id: directoryView
            model: plasmoid.nativeInterface.dirModel

            delegate: TopLevelItem {
                id: item
                property alias errorsButton: errorsButton
                property alias rescanButton: rescanButton
                property alias resumePauseButton: resumePauseButton
                property alias openButton: openButton

                ColumnLayout {
                    width: parent.width
                    spacing: 0

                    RowLayout {
                        id: itemSummary

                        PlasmaCore.IconItem {
                            Layout.preferredWidth: units.iconSizes.small
                            Layout.preferredHeight: units.iconSizes.small
                            anchors.verticalCenter: parent.verticalCenter
                            source: statusIcon
                        }
                        PlasmaComponents.Label {
                            Layout.fillWidth: true
                            anchors.verticalCenter: parent.verticalCenter
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
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Item {
                                width: units.smallSpacing
                            }
                            TinyButton {
                                id: errorsButton
                                iconSource: "emblem-important"
                                tooltip: qsTr("Show errors")
                                // 5 stands for SyncthingDirStatus::OutOfSync, unfortunately there is currently
                                // no way to expose this to QML without conflicting SyncthingStatus
                                visible: status === 5
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

                        model: DelegateModel {
                            model: plasmoid.nativeInterface.dirModel
                            rootIndex: detailsView.model.modelIndex(index)
                            delegate: DetailItem {
                            }
                        }
                    }
                }
            }
        }
    }
}
