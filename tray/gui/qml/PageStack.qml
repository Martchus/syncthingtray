import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import Qt.labs.qmlmodels

import Main

SwipeView {
    id: pageStack
    currentIndex: 0
    onCurrentIndexChanged: {
        const newIndex = pageStack.currentIndex;
        const indexHistoryLength = indexHistory.length;
        if (indexHistoryLength > 1 && indexHistory[indexHistoryLength - 2] === newIndex) {
            indexHistory.pop();
        } else {
            indexHistory.push(newIndex);
        }
        if (!goingBackAndForth) {
            indexForward.splice(0, indexForward.length);
        }
        goingBackAndForth = false;
        App.setCurrentControls(window.visible, newIndex);

        // update search text
        if (searchTextArea !== undefined) {
            pageStack.changingIndex = true;
            searchTextArea.text = pageStack.children[currentIndex]?.currentItem?.model?.filterRegularExpressionPattern() ?? "";
            pageStack.changingIndex = false;
        }
    }

    StartPage {
        id: startPage
        pages: pageStack
        onQuitRequested: closeDialog.open()
    }
    DirsPage {
        id: dirsPage
    }
    DevsPage {
        id: devsPage
    }
    ChangesPage {
        id: changesPage
    }
    AdvancedPage {
        id: advancedPage
        pages: pageStack
    }
    SettingsPage {
        id: settingsPage
    }

    required property var window
    property TextArea searchTextArea
    readonly property list<Item> children: [startPage, dirsPage, devsPage, changesPage, advancedPage, settingsPage]
    readonly property var currentPage: {
        const currentChild = children[currentIndex];
        return currentChild.currentItem ?? currentChild;
    }
    readonly property var currentDepth: children[currentIndex]?.depth ?? 1
    readonly property var currentActions: currentPage.actions ?? []
    readonly property var currentExtraActions: currentPage.extraActions ?? []
    property bool changingIndex: false
    function pop(force) {
        const currentChild = children[currentIndex];
        const currentPage = currentChild.currentItem ?? currentChild;
        if (!force && currentPage.hasUnsavedChanges) {
            const parentPage = currentPage.parentPage;
            if (parentPage?.hasUnsavedChanges !== undefined) {
                parentPage.hasUnsavedChanges = true;
                currentPage.hasUnsavedChanges = false;
                if (currentPage.isDangerous) {
                    parentPage.isDangerous = true;
                }
            } else {
                discardChangesDialog.open();
                return true;
            }
        }
        return currentPage.back?.() || currentChild.pop?.() || pageStack.back();
    }
    readonly property var indexHistory: [0]
    readonly property var indexForward: []
    property bool goingBackAndForth: false
    function back() {
        if (indexHistory.length < 1) {
            return false;
        }
        const currentIndex = indexHistory.pop();
        const previousIndex = indexHistory.pop();
        indexForward.push(currentIndex);
        goingBackAndForth = true;
        pageStack.setCurrentIndex(previousIndex);
        return true;
    }
    function forward() {
        if (indexForward.length < 1) {
            return false;
        }
        goingBackAndForth = true;
        pageStack.setCurrentIndex(indexForward.pop());
        return true;
    }
    function addDir(dirId, dirName, shareWithDeviceIds) {
        pageStack.setCurrentIndex(1);
        pageStack.currentPage.add(dirId, dirName, shareWithDeviceIds);
    }
    function addDevice(deviceId, deviceName) {
        pageStack.setCurrentIndex(2);
        pageStack.currentPage.add(deviceId, deviceName);
    }
}