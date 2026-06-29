import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import Qt.labs.qmlmodels

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
                return (event.modifiers & Qt.ShiftModifier) ? copyPathAction.trigger() : copyIdAction.trigger();
            case Qt.Key_E:
                return (event.modifiers & Qt.ShiftModifier) ? advancedConfigAction.trigger() : editAction.trigger();
            case Qt.Key_R:
                return rescanAction.trigger();
            case Qt.Key_P:
                return pauseAction.trigger();
            case Qt.Key_O:
                return openAction.trigger();
            case Qt.Key_M:
                return expandableItemDelegate.showMenu(event);
            }
        }
        actions: [
            Action {
                id: rescanAction
                text: qsTr("Rescan")
                enabled: !modelData.paused
                icon.source: QuickUI.faUrlBase + "refresh"
                onTriggered: SyncthingData.connection.rescan(modelData.dirId)
            },
            Action {
                id: pauseAction
                text: modelData.paused ? qsTr("Resume") : qsTr("Pause")
                icon.source: QuickUI.faUrlBase + (modelData.paused ? "play" : "pause")
                onTriggered: SyncthingData.connection[modelData.paused ? "resumeDirectories" : "pauseDirectories"]([modelData.dirId])
            },
            Action {
                id: openAction
                text: qsTr("Open")
                icon.source: QuickUI.faUrlBase + "folder"
                onTriggered: SyncthingModels.openPath(modelData.path)
            }
        ]
        extraActions: [
            Action {
                id: copyIdAction
                text: qsTr("Copy label/ID")
                icon.source: QuickUI.faUrlBase + "files-o"
                onTriggered: SyncthingModels.copyText(modelData.name)
            },
            Action {
                id: copyPathAction
                text: qsTr("Copy path")
                icon.source: QuickUI.faUrlBase + "files-o"
                onTriggered: SyncthingModels.copyText(modelData.path)
            },
            Action {
                id: editAction
                text: qsTr("Edit")
                icon.source: QuickUI.faUrlBase + "pencil"
                onTriggered: QuickUI.editDir(modelData.dirId, modelData.name, mainView.stackView)
            },
            Action {
                text: qsTr("Out of Sync items")
                icon.source: QuickUI.faUrlBase + "exchange"
                enabled: !modelData.paused && (modelData.neededItemsCount > 0)
                onTriggered: QuickUI.showNeededItems(modelData.dirId, modelData.name, mainView.stackView)
            },
            Action {
                text: modelData.overrideRevertActionLabel
                icon.source: QuickUI.faUrlBase + "undo"
                enabled: modelData.overrideRevertAction.length > 0
                onTriggered: mainView.confirmDirAction(modelData.dirId, modelData.name, modelData.overrideRevertAction)
            },
            Action {
                text: qsTr("Show errors")
                enabled: modelData.pullErrorCount > 0
                icon.source: QuickUI.faUrlBase + "exclamation-triangle"
                onTriggered: QuickUI.showDirErrors(modelData.dirId, modelData.name, mainView.stackView)
            },
            Action {
                text: qsTr("Ignore patterns")
                icon.source: QuickUI.faUrlBase + "filter"
                onTriggered: QuickUI.editIgnorePatterns(modelData.dirId, modelData.name, mainView.stackView)
            },
            Action {
                text: qsTr("Remote files")
                icon.source: QuickUI.faUrlBase + "folder-open-o"
                enabled: !modelData.paused
                onTriggered: QuickUI.browseFiles(modelData.dirId, modelData.name, mainView.stackView)
            },
            Action {
                id: advancedConfigAction
                text: qsTr("Advanced config")
                icon.source: QuickUI.faUrlBase + "cogs"
                onTriggered: QuickUI.editDir(modelData.dirId, modelData.name, mainView.stackView, true)
            },
            Action {
                text: qsTr("Media rescan")
                icon.source: QuickUI.faUrlBase + "music"
                enabled: SyncthingModels.scanSupported
                onTriggered: SyncthingModels.scanPath(modelData.path)
            }
        ]
    }
}
