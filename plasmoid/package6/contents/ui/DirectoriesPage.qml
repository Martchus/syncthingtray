import QtQuick 2.3
import QtQuick.Layouts 1.1
import QtQml.Models 2.2
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kirigami 2.20 as Kirigami

ColumnLayout {
    property alias view: directoryView
    property alias filter: filter
    objectName: "DirectoriesPage"

    PlasmaComponents3.TextField {
        Layout.topMargin: Kirigami.Units.smallSpacing * 2
        Layout.leftMargin: Kirigami.Units.smallSpacing * 2
        Layout.rightMargin: Kirigami.Units.smallSpacing * 2
        property bool explicitelyShown: false
        id: filter
        clearButtonShown: true
        Layout.fillWidth: true
        visible: explicitelyShown || text !== ""
        placeholderText: qsTr("Filter folders")
        onTextChanged: directoryView.model.filterRegularExpression = new RegExp(text)
    }

    PlasmaComponents3.ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true

        // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
        PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

        contentItem: TopLevelView {
            id: directoryView
            model: plasmoid.sortFilterDirModel

            delegate: TopLevelItem {
                id: item
                width: directoryView.effectiveWidth()
                readonly property string dirId_: dirId
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

                        Kirigami.Icon {
                            Layout.preferredWidth: Kirigami.Units.iconSizes.small
                            Layout.preferredHeight: Kirigami.Units.iconSizes.small
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
                                color: statusColor ? statusColor : Kirigami.Theme.textColor
                                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            }
                            Item {
                                width: Kirigami.Units.smallSpacing
                            }
                            TinyButton {
                                id: errorsButton
                                icon.source: plasmoid.faUrl + "exclamation-triangle"
                                tooltip: qsTr("Show errors")
                                visible: pullErrorCount > 0
                                onClicked: {
                                    plasmoid.showDirectoryErrors(
                                                dirId)
                                    plasmoid.expanded = false
                                }
                            }
                            TinyButton {
                                id: rescanButton
                                icon.source: plasmoid.faUrl + "refresh"
                                tooltip: qsTr("Rescan")
                                enabled: !paused
                                onClicked: plasmoid.connection.rescan(
                                               dirId)
                            }
                            TinyButton {
                                id: resumePauseButton
                                icon.source: plasmoid.faUrl + (paused ? "play" : "pause")
                                tooltip: paused ? qsTr("Resume") : qsTr("Pause")
                                onClicked: {
                                    paused ? plasmoid.connection.resumeDirectories(
                                                 [dirId]) : plasmoid.connection.pauseDirectories(
                                                 [dirId])
                                }
                            }
                            TinyButton {
                                id: openButton
                                icon.source: plasmoid.faUrl + "folder"
                                tooltip: qsTr("Open in file browser")
                                onClicked: {
                                    Qt.openUrlExternally(plasmoid.substituteTilde(path))
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
                            model: plasmoid.dirModel
                            rootIndex: directoryView.model.mapToSource(directoryView.model.index(index, 0))
                            delegate: DetailItem {
                                width: detailsView.width
                            }
                        }
                    }
                }
            }

            PlasmaExtras.Menu {
                id: contextMenu

                function init(item) {
                    // use value for properties depending on paused state from buttons
                    rescanItem.enabled = browseRemoteFilesItem.enabled = item.rescanButton.enabled
                    resumePauseItem.text = item.resumePauseButton.tooltip
                    resumePauseItem.icon = item.resumePauseButton.icon
                }

                PlasmaExtras.MenuItem {
                    text: qsTr("Copy label/ID")
                    icon: "edit-copy"
                    onClicked: directoryView.copyCurrentItemData("dirName")
                }
                PlasmaExtras.MenuItem {
                    text: qsTr("Copy path")
                    icon: "edit-copy"
                    onClicked: directoryView.copyCurrentItemData("dirPath")
                }
                PlasmaExtras.MenuItem {
                    separator: true
                }
                PlasmaExtras.MenuItem {
                    id: rescanItem
                    text: qsTr("Rescan")
                    icon: "view-refresh"
                    onClicked: directoryView.clickCurrentItemButton(
                                   "rescanButton")
                }
                PlasmaExtras.MenuItem {
                    id: resumePauseItem
                    text: qsTr("Pause")
                    icon: "media-playback-pause"
                    onClicked: directoryView.clickCurrentItemButton(
                                   "resumePauseButton")
                }
                PlasmaExtras.MenuItem {
                    id: openItem
                    text: qsTr("Open in file browser")
                    icon: "folder"
                    onClicked: directoryView.clickCurrentItemButton(
                                   "openButton")
                }
                PlasmaExtras.MenuItem {
                    id: browseRemoteFilesItem
                    text: qsTr("Browse remote files")
                    icon: "document-open-remote"
                    onClicked: directoryView.triggerNativeActionWithCurrentItemData(
                                   "browseRemoteFiles", "dirId_")
                    visible: plasmoid.wipFeaturesEnabled
                }
                PlasmaExtras.MenuItem {
                    id: showIgnorePatternsItem
                    text: qsTr("Show/edit ignore patterns")
                    icon: "document-edit"
                    onClicked: directoryView.triggerNativeActionWithCurrentItemData(
                                   "showIgnorePatterns", "dirId_")
                    visible: plasmoid.wipFeaturesEnabled
                }
            }
        }
    }
}
