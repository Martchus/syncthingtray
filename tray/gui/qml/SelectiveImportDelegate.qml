import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
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
            delegate: CheckBox {
                width: selectionView.width
                text: modelData.displayName
                onToggled: selectionView.model.setProperty(modelData.index, "checked", checked)
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
