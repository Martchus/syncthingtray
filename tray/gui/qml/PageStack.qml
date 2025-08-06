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
        pageStack.updateSearchText(pageStack.children[newIndex]?.currentItem?.model?.filterRegularExpressionPattern() ?? "", false);
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
    property string searchText
    readonly property list<Item> children: [startPage, dirsPage, devsPage, changesPage, advancedPage, settingsPage]
    readonly property var currentChild: children[currentIndex]
    readonly property var currentPage: currentChild.currentItem ?? currentChild
    readonly property var currentDepth: currentChild?.depth ?? 1
    readonly property var currentActions: currentPage.actions ?? []
    readonly property var currentExtraActions: currentPage.extraActions ?? []
    signal changesMightBeDiscarded
    function pop(force) {
        const currentChild = pageStack.currentChild;
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
                changesMightBeDiscarded();
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
    function showPage(index) {
        pageStack.setCurrentIndex(index);
        return pageStack.children[index];
    }
    function updateSearchText(searchText, updateModel = true) {
        if (pageStack.searchText === searchText) {
            return;
        }
        pageStack.searchText = searchText;
        if (!updateModel) {
            return;
        }
        const model = pageStack.children[pageStack.currentIndex]?.currentItem?.model;
        if (model?.filterRegularExpression !== undefined) {
            model.setFilterRegularExpressionPattern(searchText);
        }
    }
}
