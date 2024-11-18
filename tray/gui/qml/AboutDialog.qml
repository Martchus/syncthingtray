import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Main

CustomDialog {
    id: aboutDialog
    width: Math.min(parent.width - 20, 400)
    focus: true
    standardButtons: Dialog.Ok
    title: qsTr("About %1").arg(Qt.application.name)
    contentItem: ScrollView {
        contentWidth: availableWidth
        ColumnLayout {
            width: availableWidth
            Image {
                readonly property double size: 128
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: size
                Layout.preferredHeight: size
                source: "qrc:/icons/hicolor/scalable/app/syncthingtray.svg"
                sourceSize.width: size
                sourceSize.height: size
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                text: qsTr("Developed by %1").arg(Qt.application.organization)
                font.weight: Font.Light
                horizontalAlignment: Qt.AlignHCenter
                elide: Qt.ElideRight
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                text: qsTr("App version")
                horizontalAlignment: Qt.AlignHCenter
                elide: Qt.ElideRight
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                text: Qt.application.version
                font.weight: Font.Light
                horizontalAlignment: Qt.AlignHCenter
                elide: Qt.ElideRight
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                text: qsTr("Syncthing version")
                horizontalAlignment: Qt.AlignHCenter
                elide: Qt.ElideRight
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                text: App.syncthingVersion
                font.weight: Font.Light
                horizontalAlignment: Qt.AlignHCenter
                elide: Qt.ElideRight
            }
        }
    }
    footer: DialogButtonBox {
        Button {
            text: qsTr("License")
            flat: true
            onClicked: Qt.openUrlExternally(App.readmeUrl + "#copyright-notice-and-license")
            DialogButtonBox.buttonRole: DialogButtonBox.HelpRole
        }
        Button {
            text: qsTr("Attribution")
            flat: true
            onClicked: Qt.openUrlExternally(App.readmeUrl + "#attribution-for-3rd-party-content")
            DialogButtonBox.buttonRole: DialogButtonBox.HelpRole
        }
        Button {
            text: qsTr("Website")
            flat: true
            onClicked: Qt.openUrlExternally(App.website)
            DialogButtonBox.buttonRole: DialogButtonBox.HelpRole
        }
    }
}
