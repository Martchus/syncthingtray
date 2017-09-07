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
            model: plasmoid.nativeInterface.downloadModel

            delegate: TopLevelItem {
                id: item

                ColumnLayout {
                    width: parent.width
                    spacing: 0

                    RowLayout {
                        id: itemSummary

                        RowLayout {
                            spacing: 5
                            PlasmaComponents.Label {
                                anchors.verticalCenter: parent.verticalCenter
                                elide: Text.ElideRight
                                text: name
                            }
                        }
                        PlasmaComponents.ProgressBar {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            minimumValue: 0
                            maximumValue: 100
                            value: percentage
                        }
                        RowLayout {
                            id: toolButtonsLayout
                            spacing: 0

                            PlasmaComponents.Label {
                                height: implicitHeight
                                text: progressLabel
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Item {
                                width: 3
                            }
                            PlasmaComponents.ToolButton {
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
                        width: parent.width
                        visible: item.expanded

                        model: DelegateModel {
                            model: plasmoid.nativeInterface.downloadModel
                            rootIndex: detailsView.model.modelIndex(index)
                            delegate: RowLayout {
                                width: parent.width

                                PlasmaCore.IconItem {
                                    Layout.preferredWidth: 32
                                    Layout.preferredHeight: 32
                                    anchors.verticalCenter: parent.verticalCenter
                                    source: fileIcon
                                }
                                ColumnLayout {
                                    spacing: 0
                                    Layout.fillWidth: true
                                    RowLayout {
                                        spacing: 3
                                        Layout.fillWidth: true
                                        PlasmaComponents.Label {
                                            Layout.fillWidth: true
                                            text: name
                                            font.pointSize: 8
                                            //height: contentHeight
                                            elide: Text.ElideRight
                                        }
                                        PlasmaComponents.Label {
                                            text: progressLabel
                                            font.pointSize: 8
                                            //height: contentHeight
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
                                PlasmaComponents.ToolButton {
                                    iconSource: "folder"
                                    tooltip: qsTr("Open in file browser")
                                    onClicked: {
                                        Qt.openUrlExternally(path)
                                        plasmoid.expanded = false
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
