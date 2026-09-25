import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import Main

IconOnlyButton {
    id: helpButton
    visible: helpButton.desc.length > 0 || modelData.helpUrl?.length > 0 || (configCategory.length > 0 && !Array.isArray(configObject))
    text: qsTr("Open help")
    icon.source: QuickUI.faUrlBase + "question"
    icon.name: Utils.fallbackIconName("view-refresh")
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
                icon.name: Utils.kde ? "dialog-cancel" : ""
                flat: Utils.flatDialogButtons
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
            Button {
                text: qsTr("Details")
                icon.name: Utils.kde ? "help-contents" : ""
                flat: Utils.flatDialogButtons
                enabled: helpButton.url.toString().length > 0
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
        QuickUI.requestOpeningUrl(helpButton.url);
    }
}
