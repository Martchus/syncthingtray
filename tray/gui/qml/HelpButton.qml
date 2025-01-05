import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import Main

IconOnlyButton {
    id: helpButton
    visible: modelData.helpUrl?.length > 0 || configCategory.length > 0
    text: qsTr("Open help")
    icon.source: App.faUrlBase + "question"
    onClicked: helpButton.desc.length > 0 ? helpDlg.open() : helpButton.openSyncthingDocs()

    CustomDialog {
        id: helpDlg
        title: modelData.label ?? helpButton.key
        standardButtons: Dialog.NoButton
        contentItem: Label {
            text: helpButton.desc
            wrapMode: Text.WordWrap
        }
        footer: DialogButtonBox {
            Button {
                text: qsTr("Close")
                flat: true
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
            Button {
                text: qsTr("Details")
                flat: true
                DialogButtonBox.buttonRole: DialogButtonBox.HelpRole
            }
        }
        onHelpRequested: helpButton.openSyncthingDocs()
    }

    property string key: modelData.key
    property string desc: modelData.desc
    property string url: modelData.helpUrl ?? `https://docs.syncthing.net/users/config#${helpButton.configCategory}.${helpButton.key.toLowerCase()}`
    property string configCategory

    function openSyncthingDocs() {
        Qt.openUrlExternally(helpButton.url);
    }
}
