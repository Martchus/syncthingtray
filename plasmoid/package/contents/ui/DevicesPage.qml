import QtQuick 2.3
import QtQuick.Layouts 1.1
import QtQml.Models 2.2
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    property alias view: deviceView

    anchors.fill: parent
    objectName: "DevicesPage"

    PlasmaExtras.ScrollArea {
        anchors.fill: parent

        TopLevelView {
            id: deviceView
            model: plasmoid.nativeInterface.devModel

            delegate: TopLevelItem {
                id: item
                property alias resumePauseButton: resumePauseButton

                ColumnLayout {
                    width: parent.width
                    spacing: 0

                    RowLayout {
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
                                width: 3
                            }
                            TinyButton {
                                id: resumePauseButton
                                icon: paused ? "media-playback-start" : "media-playback-pause"
                                tooltip: paused ? qsTr("Resume") : qsTr("Pause")
                                onClicked: {
                                    paused ? plasmoid.nativeInterface.connection.resumeDevice(
                                                 [devId]) : plasmoid.nativeInterface.connection.pauseDevice(
                                                 [devId])
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
                            model: plasmoid.nativeInterface.devModel
                            rootIndex: detailsView.model.modelIndex(index)
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
                    resumePauseItem.text = item.resumePauseButton.tooltip
                    resumePauseItem.icon = item.resumePauseButton.icon
                }

                PlasmaComponents.MenuItem {
                    id: resumePauseItem
                    text: qsTr("Pause")
                    icon: "media-playback-pause"
                    onClicked: deviceView.clickCurrentItemButton(
                                   "resumePauseButton")
                }
            }
        }
    }
}
