import QtQuick 2.3
import QtQuick.Layouts 1.1
import QtQml.Models 2.2
import QtQuick.Controls 2.15 as QQC2
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core 2.0 as PlasmaCore

Item {
    property alias view: downloadView
    objectName: "DownloadsPage"

    PlasmaComponents3.ScrollView {
        anchors.fill: parent

        // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
        PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

        contentItem: TopLevelView {
            id: downloadView
            model: plasmoid.nativeInterface.downloadModel

            delegate: TopLevelItem {
                id: item
                width: downloadView.width
                readonly property string downloadName: name
                property alias openButton: openButton

                ColumnLayout {
                    width: parent.width
                    spacing: 0

                    RowLayout {
                        id: itemSummary
                        Layout.fillWidth: true

                        RowLayout {
                            spacing: units.smallSpacing
                            PlasmaComponents3.Label {
                                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                                elide: Text.ElideRight
                                text: name ? name : "?"
                            }
                        }
                        PlasmaComponents3.ProgressBar {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            from: 0.0
                            to: 100.0
                            value: percentage ? percentage : 0.0
                        }
                        RowLayout {
                            id: toolButtonsLayout
                            spacing: 0

                            PlasmaComponents3.Label {
                                height: implicitHeight
                                text: progressLabel ? progressLabel : ""
                                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            }
                            Item {
                                width: 3
                            }
                            TinyButton {
                                id: openButton
                                icon.source: "image://fa/folder"
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
                                        PlasmaComponents3.Label {
                                            Layout.fillWidth: true
                                            text: name
                                            font.pointSize: theme.defaultFont.pointSize * 0.8
                                            elide: Text.ElideRight
                                        }
                                        PlasmaComponents3.Label {
                                            text: progressLabel
                                            font.pointSize: theme.defaultFont.pointSize * 0.8
                                            elide: Text.ElideRight
                                        }
                                    }
                                    PlasmaComponents3.ProgressBar {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 8
                                        Layout.topMargin: 0
                                        from: 0.0
                                        to: 100.0
                                        value: percentage
                                    }
                                }
                                TinyButton {
                                    icon.source: "image://fa/folder"
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

            QQC2.Menu {
                id: contextMenu

                QQC2.MenuItem {
                    text: qsTr("Copy label/ID")
                    icon.name: "edit-copy"
                    onTriggered: downloadView.copyCurrentItemData("downloadName")
                }
                QQC2.MenuSeparator {
                }
                QQC2.MenuItem {
                    id: openItem
                    text: qsTr("Open in file browser")
                    icon.name: "folder"
                    onTriggered: downloadView.clickCurrentItemButton("openButton")
                }
            }
        }
    }
}
