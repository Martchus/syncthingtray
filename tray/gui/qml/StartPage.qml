import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Main

Page {
    title: qsTr("Syncthing")
    Layout.fillWidth: true
    Layout.fillHeight: true
    property var stats: App.connection.overallDirStatistics
    Flickable {
        id: mainView
        anchors.fill: parent
        ColumnLayout {
            width: mainView.width
            ItemDelegate {
                Layout.fillWidth: true
                onClicked: App.copyText(App.connection.myId)
                contentItem: RowLayout {
                    spacing: 10
                    Image {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        source: App.faUrlBase + "qrcode"
                        width: 16
                        height: 16
                    }
                    ColumnLayout {
                        Label {
                            text: qsTr("Device ID")
                        }
                        Label {
                            Layout.fillWidth: true
                            text: App.connection.myId
                            elide: Text.ElideRight
                        }
                    }
                }
            }
            ItemDelegate {
                Layout.fillWidth: true
                contentItem: ColumnLayout {
                    Label {
                        text: qsTr("Sync progress")
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        ProgressBar {
                            Layout.fillWidth: true
                            id: progressBar
                            from: 0
                            to: stats.local.total + stats.needed.total
                            value: stats.local.bytes + stats.needed.bytes
                        }
                        Label {
                            text: progressBar.position >= 1 ? qsTr("Up to Date") : qsTr("%1 %, %2 remaining").arg(Math.round(progressBar.position) * 100).arg(stats.needed.bytesAsString)
                        }
                    }
                }
            }
            ItemDelegate {
                Layout.fillWidth: true
                contentItem: RowLayout {
                    width: parent.width
                    spacing: 10
                    Image {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        source: App.faUrlBase + "globe"
                        width: 16
                        height: 16
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Global state")
                        }
                        RowLayout {
                            Image {
                                Layout.preferredWidth: 16
                                Layout.preferredHeight: 16
                                source: App.faUrlBase + "file-o"
                                width: 16
                                height: 16
                                ToolTip.text: qsTr("Files")
                                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            }
                            Label {
                                text: stats.global.files
                            }
                            Image {
                                Layout.preferredWidth: 16
                                Layout.preferredHeight: 16
                                source: App.faUrlBase + "folder-o"
                                width: 16
                                height: 16
                                ToolTip.text: qsTr("Folders")
                                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            }
                            Label {
                                text: stats.global.dirs
                            }
                            Image {
                                Layout.preferredWidth: 16
                                Layout.preferredHeight: 16
                                source: App.faUrlBase + "hdd-o"
                                width: 16
                                height: 16
                                ToolTip.text: qsTr("Size")
                                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            }
                            Label {
                                text: stats.global.bytesAsString
                            }
                        }
                    }
                }
            }
            ItemDelegate {
                Layout.fillWidth: true
                contentItem: RowLayout {
                    Layout.fillWidth: true
                    width: parent.width
                    spacing: 10
                    Image {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        source: App.faUrlBase + "home"
                        width: 16
                        height: 16
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Local state")
                        }
                        RowLayout {
                            Image {
                                Layout.preferredWidth: 16
                                Layout.preferredHeight: 16
                                source: App.faUrlBase + "file-o"
                                width: 16
                                height: 16
                                ToolTip.text: qsTr("Files")
                                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            }
                            Label {
                                text: stats.local.files
                            }
                            Image {
                                Layout.preferredWidth: 16
                                Layout.preferredHeight: 16
                                source: App.faUrlBase + "folder-o"
                                width: 16
                                height: 16
                                ToolTip.text: qsTr("Folders")
                                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            }
                            Label {
                                text: stats.local.dirs
                            }
                            Image {
                                Layout.preferredWidth: 16
                                Layout.preferredHeight: 16
                                source: App.faUrlBase + "hdd-o"
                                width: 16
                                height: 16
                                ToolTip.text: qsTr("Size")
                                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            }
                            Label {
                                text: stats.local.bytesAsString
                            }
                        }
                    }
                }
            }
        }
    }
}
