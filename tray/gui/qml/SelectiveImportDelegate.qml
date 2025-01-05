import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs

import Main

ItemDelegate {
    id: delegate
    Layout.fillWidth: true
    onClicked: {
        selectionEnabledSwitch.toggle();
        selectionEnabledSwitch.toggled();
    }
    contentItem: RowLayout {
        spacing: 15
        ForkAwesomeIcon {
            id: icon
        }
        ColumnLayout {
            Layout.fillWidth: true
            Label {
                id: mainLabel
                text: delegate.text
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.weight: Font.Medium
                wrapMode: Text.WordWrap
            }
            Label {
                id: descriptionLabel
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.weight: Font.Light
                wrapMode: Text.WordWrap
            }
        }
        Switch {
            id: selectionEnabledSwitch
            onToggled: selectionEnabledSwitch.checked && selectionDlg.open()
        }
    }
    CustomDialog {
        id: selectionDlg
        height: parent.height - 20
        standardButtons: Dialog.Ok
        contentItem: CustomListView {
            id: selectionView
            height: availableHeight
            delegate: ItemDelegate {
                width: selectionView.width
                onClicked: {
                    selectionCheckBox.toggle();
                    selectionCheckBox.toggled();
                }
                contentItem: RowLayout {
                    CheckBox {
                        id: selectionCheckBox
                        onToggled: selectionView.model.setProperty(modelData.index, "checked", checked)
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            text: modelData.displayName
                            font.weight: Font.Medium
                            elide: Text.ElideRight
                            wrapMode: Text.WordWrap
                        }
                        ItemDelegate {
                            Layout.fillWidth: true
                            visible: modelData.path !== undefined
                            height: visible ? implicitHeight : 0
                            text: modelData.path ?? ""
                            icon.source: App.faUrlBase + "folder-open-o"
                            icon.width: App.iconSize
                            icon.height: App.iconSize
                            onClicked: folderDlg.open()
                        }
                    }
                }
                FolderDialog {
                    id: folderDlg
                    title: qsTr("Set folder path of %1").arg(modelData.displayName)
                    currentFolder: "file://" + encodeURIComponent(modelData.path ?? "")
                    onAccepted: selectionView.model.setProperty(modelData.index, "path", App.resolveUrl(folderDlg.selectedFolder))
                }
                required property var modelData
            }
        }
    }
    property alias description: descriptionLabel.text
    property alias iconName: icon.iconName
    property alias dialogTitle: selectionDlg.title
    property alias model: selectionView.model
    property alias selectionEnabled: selectionEnabledSwitch.checked
}
