import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

ExpandableDelegate {
    id: mainDelegateModel
    delegate: ExpandableItemDelegate {
        id: expandableItemDelegate
        mainView: mainDelegateModel.mainView
        Keys.onPressed: (event) => {
            switch(event.key) {
            case Qt.Key_MediaTogglePlayPause:
                return pauseAction.trigger();
            case Qt.Key_Menu:
                return expandableItemDelegate.showMenu(event);
            }
            if (!(event.modifiers & Qt.ControlModifier)) {
                return;
            }
            switch(event.key) {
            case Qt.Key_C:
                return (event.modifiers & Qt.ShiftModifier) ? copyIdAction.trigger() : copyNameAction.trigger();
            case Qt.Key_E:
                return (event.modifiers & Qt.ShiftModifier) ? advancedConfigAction.trigger() : editAction.trigger();
            case Qt.Key_P:
                return pauseAction.trigger();
            case Qt.Key_M:
                return expandableItemDelegate.showMenu(event);
            }
        }
        actions: [
            Action {
                id: pauseAction
                text: modelData.paused ? qsTr("Resume") : qsTr("Pause")
                enabled: !modelData.isThisDevice
                icon.source: QuickUI.faUrlBase + (modelData.paused ? "play" : "pause")
                onTriggered: SyncthingData.connection[modelData.paused ? "resumeDevice" : "pauseDevice"]([modelData.devId])
            }
        ]
        extraActions: [
            Action {
                id: copyNameAction
                text: qsTr("Copy name")
                icon.source: QuickUI.faUrlBase + "files-o"
                onTriggered: SyncthingModels.copyText(modelData.name)
            },
            Action {
                id: copyIdAction
                text: qsTr("Copy ID")
                icon.source: QuickUI.faUrlBase + "files-o"
                onTriggered: SyncthingModels.copyText(modelData.devId)
            },
            Action {
                id: editAction
                text: qsTr("Edit")
                icon.source: QuickUI.faUrlBase + "pencil"
                onTriggered: QuickUI.editDev(modelData.devId, modelData.name, mainView.stackView)
            },
            Action {
                text: qsTr("Out of Sync items")
                icon.source: QuickUI.faUrlBase + "exchange"
                enabled: !modelData.paused && (modelData.neededItemsCount > 0)
                onTriggered: QuickUI.showOutOfSyncDirs(modelData.devId, modelData.name, modelData.index, mainDelegateModel.model, mainView.stackView)
            },
            Action {
                id: advancedConfigAction
                text: qsTr("Advanced config")
                icon.source: QuickUI.faUrlBase + "cogs"
                onTriggered: QuickUI.editDev(modelData.devId, modelData.name, mainView.stackView, true)
            }
        ]
    }
}
