import QtQuick 2.3
import QtQuick.Layouts 1.1
import QtQml.Models 2.2
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kirigami 2.20 as Kirigami

Item {
    property alias view: deviceView
    objectName: "DevicesPage"

    PlasmaComponents3.ScrollView {
        anchors.fill: parent

        // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
        PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

        contentItem: TopLevelView {
            id: deviceView
            model: plasmoid.sortFilterDevModel

            delegate: TopLevelItem {
                id: item
                width: deviceView.effectiveWidth()
                readonly property string devName: name
                readonly property string devID: devId
                property alias resumePauseButton: resumePauseButton

                ColumnLayout {
                    width: parent.width
                    spacing: 0

                    RowLayout {
                        Layout.fillWidth: true

                        PlasmaCore.IconItem {
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
                                color: statusColor ? statusColor : PlasmaCore.ColorScope.textColor
                                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            }
                            Item {
                                width: 3
                            }
                            TinyButton {
                                id: resumePauseButton
                                icon.source: plasmoid.faUrl + (paused ? "play" : "pause")
                                icon.cache: false
                                tooltip: paused ? qsTr("Resume") : qsTr("Pause")
                                enabled: !isOwnDevice
                                onClicked: {
                                    paused ? plasmoid.connection.resumeDevice(
                                                 [devId]) : plasmoid.connection.pauseDevice(
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
                            model: plasmoid.devModel
                            rootIndex: deviceView.model.mapToSource(deviceView.model.index(index, 0))
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
                    resumePauseItem.text = item.resumePauseButton.tooltip
                    resumePauseItem.icon = item.resumePauseButton.icon
                }

                PlasmaExtras.MenuItem {
                    text: qsTr("Copy name")
                    icon: "edit-copy"
                    onClicked: deviceView.copyCurrentItemData("devName")
                }
                PlasmaExtras.MenuItem {
                    text: qsTr("Copy ID")
                    icon: "edit-copy"
                    onClicked: deviceView.copyCurrentItemData("devID")
                }
                PlasmaExtras.MenuItem {
                    separator: true
                }
                PlasmaExtras.MenuItem {
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
