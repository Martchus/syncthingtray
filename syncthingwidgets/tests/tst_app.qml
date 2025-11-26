import QtQuick
import QtQuick.Layouts
import QtTest
import Main


Item {
    id: root
    anchors.fill: parent
    LeftDrawer {
        id: drawer
        pageStack: pageStack
        aboutDialog: aboutDialog
        closeDialog: closeDialog
    }
    ColumnLayout {
        anchors.fill: parent
        MainToolBar {
            Layout.fillWidth: true
            drawer: drawer
            pageStack: pageStack
        }
        PageStack {
            Layout.fillWidth: true
            Layout.fillHeight: true
            id: pageStack
            window: root.parent
            onChangesMightBeDiscarded: discardChangesDialog.visible = true
        }
        MainTabBar {
            id: toolBar
            Layout.fillWidth: true
            drawer: drawer
            pageStack: pageStack
        }
    }
    AboutDialog {
        id: aboutDialog
    }
    CloseDialog {
        id: closeDialog
        meta: control.meta
        onCloseRequested: testCase.closeRequested = true
    }
    DiscardChangesDialog {
        id: discardChangesDialog
        meta: control.meta
        pageStack: pageStack
    }

    readonly property Notifications notifications: Notifications {
        pageStack: pageStack
    }
    readonly property Theming theming: Theming {
        pageStack: pageStack
    }
    readonly property Meta meta: Meta {
    }
    property bool closeRequested

    TestCase {
        id: testCase
        name: "AppTests"

        function initTestCase() {
            compare(App.connection.connected, withSyncthing, "connected");
        }

        function cleanup() {
            while (pageStack.pop(true)) { }
        }

        function goBackToStartPage() {
            pageStack.pop(true);
            compare(pageStack.currentPage.title, "Syncthing", "back on start page");
            compare(drawer.currentItem.name, "Start", "drawer index updated to start page");
            compare(toolBar.currentIndex, 0, "tab bar index updated to start page");
        }

        function test_miscProperties() {
            compare(meta.title, "Syncthing", "title set");
            verify(theming.font, "font set");
        }

        function test_pageSwitching() {
            compare(pageStack.currentPage.title, "Syncthing", "start page shown initially");
            compare(drawer.currentItem.name, "Start", "drawer index initialized for start page");
            compare(toolBar.currentIndex, 0, "tab bar index initialized for start page");

            pageStack.showPage(1);
            compare(pageStack.currentPage.title, "Folders", "folders page shown");
            compare(drawer.currentItem.name, "Folders", "drawer index updated for folders page");
            compare(toolBar.currentIndex, 1, "tab bar index updated for folders page");

            pageStack.showPage(2);
            compare(pageStack.currentPage.title, "Devices", "devices page shown");
            compare(drawer.currentItem.name, "Devices", "drawer index updated for devices page");
            compare(toolBar.currentIndex, 2, "tab bar index updated for devices page");

            pageStack.showPage(3);
            compare(pageStack.currentPage.title, "Recent changes", "changes page shown");
            compare(drawer.currentItem.name, "Recent changes", "drawer index updated for changes page");
            compare(toolBar.currentIndex, 3, "tab bar index updated for changes page");

            pageStack.showPage(4);
            compare(pageStack.currentPage.title, "Advanced", "advanced page shown");
            compare(drawer.currentItem.name, "Advanced", "drawer index updated for advanced page");
            compare(toolBar.currentIndex, 4, "tab bar index updated for advanced page");

            pageStack.showPage(5);
            compare(pageStack.currentPage.title, "App settings", "app settings page shown");
            compare(drawer.currentItem.name, "App settings", "drawer index updated for app settings");
            compare(toolBar.currentIndex, 4, "tab bar index not changed for app settings");

            pageStack.pop();
            compare(pageStack.currentPage.title, "Advanced", "can go back to previous page");
            compare(drawer.currentItem.name, "Advanced", "drawer index updated when going back");
            compare(toolBar.currentIndex, 4, "tab bar index unchanged when going back from app to advanced settings");

            pageStack.pop();
            compare(pageStack.currentPage.title, "Recent changes", "can go back further");
            compare(drawer.currentItem.name, "Recent changes", "drawer index updated when going back further");
            compare(toolBar.currentIndex, 3, "tab bar index updated when going back further to recent changes");

            pageStack.forward();
            compare(pageStack.currentPage.title, "Advanced", "can go forward again");
            compare(drawer.currentItem.name, "Advanced", "drawer index updated when going forward");
            compare(toolBar.currentIndex, 4, "tab bar index updated when going forward");

            pageStack.forward();
            compare(pageStack.currentPage.title, "App settings", "can go forward again (2)");
            compare(drawer.currentItem.name, "App settings", "drawer index updated when going forward (2)");
        }

        function test_addingFolderFromStartPage() {
            pageStack.showPage(0);
            pageStack.addDir();

            const addFolderPage = pageStack.currentPage;
            compare(addFolderPage.title, "Add new folder", "page for adding new folder shown");
            compare(addFolderPage.existing, false, "showing folder config page for new folder");
            compare(addFolderPage.isDangerous, false, "adding new folder not considered dangerous");
            compare(drawer.currentItem.name, "Folders", "drawer index updated to folders page");
            compare(toolBar.currentIndex, 1, "tab bar index updated for folders page");

            const newPath = settingsPath + "/testfolder";
            if (withSyncthing) {
                const folderModel = addFolderPage.model;
                verify(folderModel.count > 2, "model populated");
                compare(folderModel.get(0).label, "ID", "ID label present");
                compare(folderModel.get(1).label, "Label", "label label present");
                compare(folderModel.get(2).label, "Paused", "paused label present");
                compare(folderModel.get(2).value, true, "paused enabled by default");
                compare(folderModel.get(3).label, "Path", "path label present");
                compare(folderModel.get(3).value, "", "default path present");

                // open dialog to edit ID
                const listView = pageStack.currentPage.listView;
                listView.currentIndex = 0;
                const firstItem = pageStack.currentPage.listView.currentItem; // select ID
                firstItem.click();
                verify(firstItem.dialog.visible, "dialog to edit ID is open");
                compare(firstItem.dialog.title, "ID", "dialog title set");

                // wait until random ID is loaded
                tryVerify(() => folderModel.get(0).value?.length === 11, 5000, "random ID present");
                const randomID = folderModel.get(0).value;
                compare(firstItem.dialog.text, randomID);

                // edit text but reject dialog
                firstItem.dialog.text = "foo";
                firstItem.dialog.reject();
                verify(firstItem.dialog.visible, "dialog to edit ID is closed");
                compare(folderModel.get(0).value, randomID, "random ID still assigned after rejecting dialog");

                // edit text and accept dialog
                firstItem.click();
                firstItem.dialog.text = "foo";
                firstItem.dialog.accept();
                verify(firstItem.dialog.visible, "dialog to edit ID is closed");
                compare(folderModel.get(0).value, "foo", "entered value has been set");

                // going back with unsaved changes should trigger dialog
                verify(!discardChangesDialog.visible);
                pageStack.pop();
                verify(discardChangesDialog.visible);
                discardChangesDialog.reject();

                // open dialog to edit path
                listView.currentIndex = 3;
                const pathItem = pageStack.currentPage.listView.currentItem; // select ID
                pathItem.manualButton.click();
                verify(pathItem.dialog.visible, "dialog to edit path is open");
                compare(pathItem.dialog.title, "Path", "dialog title set");

                // edit path and accept dialog
                pathItem.dialog.text = newPath;
                pathItem.dialog.accept();
                verify(pathItem.dialog.visible, "dialog to edit path is closed");
                compare(folderModel.get(3).value, newPath, "entered path has been set");

                // add folder
                const applyAction = pageStack.currentActions[2];
                discardChangesDialog.close();
                tryVerify(() => !discardChangesDialog.visible);
                compare(applyAction.text, "Apply");
                applyAction.trigger();

                // going back shouldn't trigger dialog
                wait(1); // saving might be triggered asynchronously
                tryVerify(() => !App.savingConfig);
                goBackToStartPage();
            }

            goBackToStartPage();
            verify(!discardChangesDialog.visible);

            if (withSyncthing) {
                pageStack.showPage(1);
                const model = pageStack.currentPage.model;
                tryVerify(() => model.rowCount() === 1, 5000, "new folder present");
                const firstFolderIndex = model.index(0, 0);
                compare(model.data(firstFolderIndex, directoryIdRole), "foo", "new folder ID present");
                compare(model.data(firstFolderIndex, directoryPathRole), newPath, "new folder path present");
            }
        }

        function test_addingDeviceFromStartPage() {
            pageStack.showPage(0);
            pageStack.addDevice();

            const addDevicePage = pageStack.currentPage;
            compare(addDevicePage.title, "Add new device", "page for adding new device shown");
            compare(addDevicePage.existing, false, "showing device config page for new device");
            compare(addDevicePage.isDangerous, false, "adding new device not considered dangerous");
            if (withSyncthing) {
                const deviceModel = addDevicePage.model;
                verify(deviceModel.count > 0, "model populated");
                compare(deviceModel.get(0).label, "Device ID", "paused label present");
                compare(deviceModel.get(0).value, "", "paused enabled by default");
            }

            compare(drawer.currentItem.name, "Devices", "drawer index updated to devices page");
            compare(toolBar.currentIndex, 2, "tab bar index updated for devices page");

            goBackToStartPage();
        }

        function test_devicesPage() {
            if (!withSyncthing) {
                skip("not testing devices page without having Syncthing started");
                return;
            }
            pageStack.showPage(2);
            const model = pageStack.currentPage.model;
            tryVerify(() => model.rowCount() === 1, 5000, "this device present");
            const thisDeviceIndex = model.index(0, 0);
            compare(model.data(thisDeviceIndex, deviceStatusStringRole), "This Device", "device status present");
        }

        function test_recentChangesPage() {
            if (!withSyncthing) {
                skip("not testing recent changes page without having Syncthing started");
                return;
            }
            pageStack.showPage(3);
            const model = pageStack.currentPage.model;
            tryCompare(model, "count", 0, 5000, "no recent changes present initially");
        }
    }
}
