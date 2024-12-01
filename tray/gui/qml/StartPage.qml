import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

import Main

Page {
    id: startPage
    title: qsTr("Syncthing")
    Layout.fillWidth: true
    Layout.fillHeight: true
    property var stats: App.connection.overallDirStatistics
    CustomFlickable {
        id: mainView
        anchors.fill: parent
        contentHeight: mainLayout.height
        ColumnLayout {
            id: mainLayout
            width: mainView.width
            spacing: 0
            ItemDelegate {
                Layout.fillWidth: true
                contentItem: RowLayout {
                    spacing: 15
                    ForkAwesomeIcon {
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                        iconName: "download"
                    }
                    ColumnLayout {
                        Label {
                            text: qsTr("Local sync progress")
                            elide: Text.ElideRight
                            font.weight: Font.Medium
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
                                font.weight: Font.Light
                            }
                        }
                    }
                }
            }
            ItemDelegate {
                Layout.fillWidth: true
                contentItem: RowLayout {
                    spacing: 15
                    Icon {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        Layout.maximumWidth: 16
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                        width: 16
                        height: 16
                        source: App.statusIcon
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Status")
                            elide: Text.ElideRight
                            font.weight: Font.Medium
                        }
                        Label {
                            Layout.fillWidth: true
                            text: App.statusText
                            font.weight: Font.Light
                            wrapMode: Text.Wrap
                        }
                        Label {
                            Layout.fillWidth: true
                            text: App.additionalStatusText
                            font.weight: Font.Light
                            wrapMode: Text.Wrap
                            visible: text.length > 0
                        }
                    }
                }
            }
            ItemDelegate {
                Layout.fillWidth: true
                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    onTapped: qrCodeDlg.open()
                    onLongPressed: {
                        App.copyText(App.connection.myId);
                        App.performHapticFeedback();
                    }
                }
                contentItem: RowLayout {
                    spacing: 15
                    ForkAwesomeIcon {
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                        iconName: "qrcode"
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Own device ID")
                            elide: Text.ElideRight
                            font.weight: Font.Medium
                        }
                        Label {
                            Layout.fillWidth: true
                            text: App.connection.myId
                            elide: Text.ElideRight
                            wrapMode: Text.Wrap
                            font.weight: Font.Light
                        }
                    }
                }
                CustomDialog {
                    id: qrCodeDlg
                    title: qsTr("Own device ID")
                    standardButtons: Dialog.NoButton
                    width: parent.width - 20
                    height: Math.min(parent.height - 20, width)
                    onAboutToShow: App.showQrCode(qrCodeIcon)
                    contentItem: Icon {
                        id: qrCodeIcon
                    }
                    footer: DialogButtonBox {
                        Button {
                            text: qsTr("Copy as text")
                            flat: true
                            onClicked: App.copyText(App.connection.myId)
                        }
                        Button {
                            text: qsTr("Close")
                            flat: true
                            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                        }
                    }
                }
            }
            ItemDelegate {
                Layout.fillWidth: true
                contentItem: RowLayout {
                    spacing: 15
                    ForkAwesomeIcon {
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                        iconName: "tachometer"
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Traffic")
                            elide: Text.ElideRight
                            font.weight: Font.Medium
                        }
                        RowLayout {
                            ForkAwesomeIcon {
                                iconName: "cloud-download"
                            }
                            Label {
                                Layout.fillWidth: true
                                text: App.formatTraffic(App.connection.totalIncomingTraffic, App.connection.totalIncomingRate)
                                elide: Text.ElideRight
                                wrapMode: Text.Wrap
                                font.weight: Font.Light
                            }
                        }
                        RowLayout {
                            ForkAwesomeIcon {
                                iconName: "cloud-upload"
                            }
                            Label {
                                Layout.fillWidth: true
                                text: App.formatTraffic(App.connection.totalOutgoingTraffic, App.connection.totalOutgoingRate)
                                elide: Text.ElideRight
                                wrapMode: Text.Wrap
                                font.weight: Font.Light
                            }
                        }
                    }
                }
            }
            Statistics {
                stats: startPage.stats.global
                labelText: qsTr("Global state")
                iconName: "globe"
            }
            Statistics {
                stats: startPage.stats.local
                labelText: qsTr("Local state")
                iconName: "home"
            }
            ItemDelegate {
                Layout.fillWidth: true
                onClicked: {
                    pages.setCurrentIndex(2);
                    pages.currentPage.add();
                }
                contentItem: RowLayout {
                    spacing: 15
                    ForkAwesomeIcon {
                        iconName: "laptop"
                    }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Connect other device")
                        elide: Text.ElideRight
                        font.weight: Font.Medium
                    }
                }
            }
            ItemDelegate {
                Layout.fillWidth: true
                onClicked: {
                    pages.setCurrentIndex(1);
                    pages.currentPage.add();
                }
                contentItem: RowLayout {
                    spacing: 15
                    ForkAwesomeIcon {
                        iconName: "share-alt"
                    }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Share folder")
                        elide: Text.ElideRight
                        font.weight: Font.Medium
                    }
                }
            }
            ItemDelegate {
                Layout.fillWidth: true
                onClicked: Qt.openUrlExternally(App.connection.syncthingUrlWithCredentials)
                contentItem: RowLayout {
                    spacing: 15
                    ForkAwesomeIcon {
                        iconName: "external-link"
                    }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Open Syncthing in web browser")
                        elide: Text.ElideRight
                        font.weight: Font.Medium
                    }
                }
            }
        }
    }
    required property var pages
}
