import QtQuick 2.3
import QtQuick.Layouts 1.1
import QtQml.Models 2.2
import QtQuick.Controls 2.15 as QQC2
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
                                icon.source: "image://fa/exclamation-triangle"
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
                                icon.source: "image://fa/refresh"
                                tooltip: qsTr("Rescan")
                                enabled: !paused
                                onClicked: plasmoid.nativeInterface.connection.rescan(
                                               dirId)
                            }
                            TinyButton {
                                id: resumePauseButton
                                icon.source: paused ? "image://fa/play" : "image://fa/pause"
                                tooltip: paused ? qsTr("Resume") : qsTr("Pause")
                                onClicked: {
                                    paused ? plasmoid.nativeInterface.connection.resumeDirectories(
                                                 [dirId]) : plasmoid.nativeInterface.connection.pauseDirectories(
                                                 [dirId])
                                }
                            }
                            TinyButton {
                                id: openButton
                                icon.source: "image://fa/folder"
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
                            rootIndex: directoryView.model.mapToSource(directoryView.model.index(index, 0))
                            delegate: DetailItem {
                                width: detailsView.width
                            }
                        }
                    }
                }
            }

            QQC2.Menu {
                id: contextMenu

                function init(item) {
                    // use value for properties depending on paused state from buttons
                    rescanItem.enabled = item.rescanButton.enabled
                    resumePauseItem.text = item.resumePauseButton.tooltip
                    resumePauseItem.icon.name = item.resumePauseButton.icon
                }

                QQC2.MenuItem {
                    text: qsTr("Copy label/ID")
                    icon.name: "edit-copy"
                    onTriggered: directoryView.copyCurrentItemData("dirName")
                }
                QQC2.MenuItem {
                    text: qsTr("Copy path")
                    icon.name: "edit-copy"
                    onTriggered: directoryView.copyCurrentItemData("dirPath")
                }
                QQC2.MenuSeparator {
                }
                QQC2.MenuItem {
                    id: rescanItem
                    text: qsTr("Rescan")
                    icon.name: "view-refresh"
                    onTriggered: directoryView.clickCurrentItemButton(
                                   "rescanButton")
                }
                QQC2.MenuItem {
                    id: resumePauseItem
                    text: qsTr("Pause")
                    icon.name: "media-playback-pause"
                    onTriggered: directoryView.clickCurrentItemButton(
                                   "resumePauseButton")
                }
                QQC2.MenuItem {
                    id: openItem
                    text: qsTr("Open in file browser")
                    icon.name: "folder"
                    onTriggered: directoryView.clickCurrentItemButton(
                                   "openButton")
                }
            }
        }
    }
}
