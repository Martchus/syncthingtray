import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

ExpandableListView {
    id: mainView
    model: DirDelegate {
        mainView: mainView
    }
    CustomDialog {
        id: confirmDirActionDlg
        title: isOverrideAction ? qsTr("Override changes on remote devices") : qsTr("Revert local changes")
        standardButtons: Dialog.Ok | Dialog.Cancel
        Material.primary: Material.Red
        Material.accent: Material.primary
        contentItem: Label {
            text: confirmDirActionDlg.isOverrideAction ? qsTr("Do you really want to override changes on remote devices within folder \"%1\"? This will mark the local version as the latest version causing changes on all remote devices to be overridden with the version from this device.").arg(confirmDirActionDlg.dirLabel)
                                                       : qsTr("Do you really want to revert the local changes on this device within folder \"%1\"? This will undo all local changes on this device.").arg(confirmDirActionDlg.dirLabel)
            elide: Label.ElideRight
            wrapMode: Text.WordWrap
        }
        onAccepted: App.invokeDirAction(confirmDirActionDlg.dirId, confirmDirActionDlg.action)
        property string dirId
        property string dirLabel
        property string action
        readonly property bool isOverrideAction: action === "override"
    }
    function confirmDirAction(dirId, dirLabel, action) {
        confirmDirActionDlg.dirId = dirId;
        confirmDirActionDlg.dirLabel = dirLabel;
        confirmDirActionDlg.action = action;
        confirmDirActionDlg.open();
    }
}
