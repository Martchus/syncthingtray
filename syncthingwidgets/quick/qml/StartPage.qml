import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs

import Main

Page {
    id: startPage
    title: qsTr("Syncthing")
    Layout.fillWidth: true
    Layout.fillHeight: true
    property var stats: SyncthingData.connection.overallDirStatistics
    property var remoteCompletion: SyncthingData.connection.overallRemoteCompletion
    signal quitRequested
    CustomFlickable {
        id: mainView
        anchors.fill: parent
        contentHeight: mainLayout.height
        ColumnLayout {
            id: mainLayout
            width: mainView.width
            spacing: 0
            SectionHeader {
                section: qsTr("Pending setup tasks")
                visible: storagePermissionDelegate.visible || notificationPermissionDelegate.visible || authSetupDelegate.visible
            }
            CustomDelegate {
                id: storagePermissionDelegate
                onClicked: App.requestStoragePermission()
                visible: !App.storagePermissionGranted
                labelText: qsTr("Request storage permission")
                iconName: "unlock-alt"
            }
            CustomDelegate {
                id: notificationPermissionDelegate
                onClicked: App.requestNotificationPermission()
                visible: !App.notificationPermissionGranted
                labelText: qsTr("Request notification permission")
                iconName: "bell"
            }
            ItemDelegate {
                id: authSetupDelegate
                Layout.fillWidth: true
                onClicked: startPage.pages.showPage(4).showGuiAuth()
                visible: !App.usingUnixDomainSocket && SyncthingData.connection.hasState && !SyncthingData.connection.guiRequiringAuth
                contentItem: RowLayout {
                    spacing: 15
                    ForkAwesomeIcon {
                        iconName: "key-modern"
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Set password for web-based GUI")
                            elide: Text.ElideRight
                            font.weight: Font.Medium
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Otherwise other apps can access the web-based GUI without authentication.")
                            font.weight: Font.Light
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
            SectionHeader {
                section: qsTr("Status and statistics")
            }
            ItemDelegate {
                Layout.fillWidth: true
                visible: SyncthingData.connection.hasState
                contentItem: RowLayout {
                    spacing: 15
                    ForkAwesomeIcon {
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                        iconName: "download"
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Local sync progress")
                            wrapMode: Text.Wrap
                            font.weight: Font.Medium
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            ProgressBar {
                                Layout.fillWidth: true
                                id: progressBar
                                from: 0
                                to: 100
                                value: stats.completionPercentage
                            }
                            Label {
                                text: progressBar.position >= 1 ? qsTr("Up to Date") : qsTr("%1 %, %2 remaining").arg(Math.floor(progressBar.position * 100)).arg(stats.needed.bytesAsString)
                                font.weight: Font.Light
                            }
                        }
                    }
                }
            }
            ItemDelegate {
                Layout.fillWidth: true
                visible: SyncthingData.connection.hasState
                contentItem: RowLayout {
                    spacing: 15
                    ForkAwesomeIcon {
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                        iconName: "upload"
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Remote sync progress (of connected devices)")
                            wrapMode: Text.Wrap
                            font.weight: Font.Medium
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            ProgressBar {
                                Layout.fillWidth: true
                                id: remoteProgressBar
                                from: 0
                                to: 100
                                value: remoteCompletion.percentage
                            }
                            Label {
                                text: remoteProgressBar.position >= 1 ? qsTr("Up to Date") : (Number.isNaN(remoteCompletion.percentage) ? qsTr("Not available") : qsTr("%1 %").arg(Math.round(remoteCompletion.percentage)))
                                font.weight: Font.Light
                            }
                        }
                    }
                }
            }
            ItemDelegate {
                Layout.fillWidth: true
                onClicked: {
                    const connection = SyncthingData.connection;
                    if (!connection.connected && !connection.connecting) {
                        connection.connect();
                    } else if (connection.hasErrors) {
                        startPage.pages.showPage(5).push("ErrorsPage.qml", {}, StackView.PushTransition);
                    } else {
                        QuickUI.performHapticFeedback();
                    }
                }
                onPressAndHold: App.reconnectToSyncthing()
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
                visible: SyncthingData.connection.hasState
                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    onTapped: qrCodeDlg.open()
                    onLongPressed: {
                        SyncthingModels.copyText(SyncthingData.connection.myId);
                        QuickUI.performHapticFeedback();
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
                            text: SyncthingData.connection.myId
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
                    implicitWidth: 400
                    implicitHeight: implicitWidth
                    onAboutToShow: SyncthingModels.showQrCode(qrCodeIcon)
                    contentItem: Icon {
                        id: qrCodeIcon
                    }
                    footer: DialogButtonBox {
                        Button {
                            text: qsTr("Copy as text")
                            flat: true
                            onClicked: SyncthingModels.copyText(SyncthingData.connection.myId)
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
                visible: SyncthingData.connection.hasState
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
                                text: SyncthingModels.formatTraffic(SyncthingData.connection.totalIncomingTraffic, SyncthingData.connection.totalIncomingRate)
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
                                text: SyncthingModels.formatTraffic(SyncthingData.connection.totalOutgoingTraffic, SyncthingData.connection.totalOutgoingRate)
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
            SectionHeader {
                section: qsTr("Getting started")
            }
            CustomDelegate {
                onClicked: pages.addDevice()
                visible: SyncthingData.connection.hasState
                labelText: qsTr("Connect other device")
                iconName: "laptop"
            }
            CustomDelegate {
                onClicked: pages.addDir()
                visible: SyncthingData.connection.hasState
                labelText: qsTr("Share folder")
                iconName: "share-alt"
            }
            CustomDelegate {
                onClicked: SyncthingModels.openUrlExternally(SyncthingData.connection.syncthingUrlWithCredentials)
                visible: !App.usingUnixDomainSocket
                labelText: qsTr("Open Syncthing in web browser")
                iconName: "external-link"
            }
            CustomDelegate {
                onClicked: SyncthingModels.openUrlExternally(SyncthingData.documentationUrl)
                labelText: qsTr("Open documentation")
                iconName: "book"
            }
        }
    }
    required property var pages
    property list<Action> actions: [
        Action {
            text: qsTr("Restart Syncthing")
            icon.source: QuickUI.faUrlBase + "refresh"
            onTriggered: App.restartSyncthing()
        },
        Action {
            text: qsTr("Quit app")
            icon.source: QuickUI.faUrlBase + "power-off"
            onTriggered: startPage.quitRequested()
        }
    ]
}
