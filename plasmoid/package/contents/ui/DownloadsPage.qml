import QtQuick 2.3
import QtQuick.Layouts 1.1
import QtQml.Models 2.2
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    property alias view: downloadView
    anchors.fill: parent

    objectName: "DownloadsPage"

    PlasmaExtras.ScrollArea {
        anchors.fill: parent

        TopLevelView {
            id: downloadView
            width: parent.width
            model: plasmoid.nativeInterface.downloadModel

            delegate: TopLevelItem {
                id: item
                width: downloadView.width
                property alias openButton: openButton

                ColumnLayout {
                    width: parent.width
                    spacing: 0

                    RowLayout {
                        id: itemSummary
                        Layout.fillWidth: true

                        RowLayout {
                            spacing: units.smallSpacing
                            PlasmaComponents.Label {
                                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                                elide: Text.ElideRight
                                text: name ? name : "?"
                            }
                        }
                        PlasmaComponents.ProgressBar {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            minimumValue: 0
                            maximumValue: 100
                            value: percentage ? percentage : 0
                        }
                        RowLayout {
                            id: toolButtonsLayout
                            spacing: 0

                            PlasmaComponents.Label {
                                height: implicitHeight
                                text: progressLabel ? progressLabel : ""
                                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            }
                            Item {
                                width: 3
                            }
                            TinyButton {
                                id: openButton
                                icon: "folder"
                                tooltip: qsTr("Open in file browser")
                                enabled: path !== undefined
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

                        model: DelegateModel {
                            model: plasmoid.nativeInterface.downloadModel
                            rootIndex: detailsView.model.modelIndex(index)
                            delegate: RowLayout {
                                width: detailsView.width

                                PlasmaCore.IconItem {
                                    Layout.preferredWidth: units.iconSizes.medium
                                    Layout.preferredHeight: units.iconSizes.medium
                                    Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                                    source: fileIcon
                                }
                                ColumnLayout {
                                    spacing: 0
                                    Layout.fillWidth: true
                                    RowLayout {
                                        spacing: units.smallSpacing
                                        Layout.fillWidth: true
                                        PlasmaComponents.Label {
                                            Layout.fillWidth: true
                                            text: name
                                            font.pointSize: theme.defaultFont.pointSize * 0.8
                                            elide: Text.ElideRight
                                        }
                                        PlasmaComponents.Label {
                                            text: progressLabel
                                            font.pointSize: theme.defaultFont.pointSize * 0.8
                                            elide: Text.ElideRight
                                        }
                                    }
                                    PlasmaComponents.ProgressBar {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 8
                                        Layout.topMargin: 0
                                        minimumValue: 0
                                        maximumValue: 100
                                        value: percentage
                                    }
                                }
                                TinyButton {
                                    icon: "folder"
                                    tooltip: qsTr("Open in file browser")
                                    onClicked: {
                                        Qt.openUrlExternally(path + "/..")
                                        plasmoid.expanded = false
                                    }
                                }
                            }
                        }
                    }
                }
            }

            PlasmaComponents.Menu {
                id: contextMenu

                PlasmaComponents.MenuItem {
                    id: openItem
                    text: qsTr('Open in file browser')
                    icon: "folder"
                    onClicked: downloadView.clickCurrentItemButton("openButton")
                }
            }
        }
    }
}
