import QtQuick
import QtQuick.Controls.Material
import QtQuick.Dialogs
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
        onNotification: (message) => {
            setup.debug("Notification: ", message);
            messages.push(message);
        }
    }
    readonly property Theming theming: Theming {
        pageStack: pageStack
    }
    readonly property Meta meta: Meta {
    }
    property bool closeRequested
    property int foldersAdded: 0
    property list<string> messages

    readonly property int optionsIndex: 6
    readonly property int importIndex: 7
    readonly property int exportIndex: importIndex + 1
    readonly property int localDiscoveryIndex: 7
    readonly property int globalDiscoveryIndex: 8

    TestCase {
        id: testCase
        name: "AppTests"

        function initTestCase() {
            const connection = App.connection;
            compare(connection.connected, withSyncthing, "connected");
            if (withSyncthing) {
                const folders = connection.rawConfig.folders;
                verify(Array.isArray(folders), "folders present");
                foldersAdded = folders.length;
            }
            verify(foldersAdded <= 1, "zero or one folder(s) initially present");
        }

        function cleanup() {
            while (pageStack.pop(true)) { }
            pageStack.resetHistory();
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
            compare(pageStack.serialize(), {"index": 0, "indexHistory": [], "indexForward": [], "setPageHistory": [], "children": [undefined, undefined, undefined, undefined, undefined, undefined]});

            pageStack.showPage(1);
            compare(pageStack.currentPage.title, "Folders", "folders page shown");
            compare(drawer.currentItem.name, "Folders", "drawer index updated for folders page");
            compare(toolBar.currentIndex, 1, "tab bar index updated for folders page");
            compare(pageStack.serialize(), {"index": 1, "indexHistory": [1], "indexForward": [], "setPageHistory": [1], "children": [undefined, undefined, undefined, undefined, undefined, undefined]});

            pageStack.showPage(2);
            compare(pageStack.currentPage.title, "Devices", "devices page shown");
            compare(drawer.currentItem.name, "Devices", "drawer index updated for devices page");
            compare(toolBar.currentIndex, 2, "tab bar index updated for devices page");
            compare(pageStack.serialize(), {"index": 2, "indexHistory": [1, 2], "indexForward": [], "setPageHistory": [1, 2], "children": [undefined, undefined, undefined, undefined, undefined, undefined]});

            pageStack.showPage(3);
            compare(pageStack.currentPage.title, "Recent changes", "changes page shown");
            compare(drawer.currentItem.name, "Recent changes", "drawer index updated for changes page");
            compare(toolBar.currentIndex, 3, "tab bar index updated for changes page");
            compare(pageStack.serialize(), {"index": 3, "indexHistory": [1, 2, 3], "indexForward": [], "setPageHistory": [1, 2, 3], "children": [undefined, undefined, undefined, undefined, undefined, undefined]});

            pageStack.showPage(4);
            compare(pageStack.currentPage.title, "Advanced", "advanced page shown");
            compare(drawer.currentItem.name, "Advanced", "drawer index updated for advanced page");
            compare(toolBar.currentIndex, 4, "tab bar index updated for advanced page");
            compare(pageStack.serialize(), {"index": 4, "indexHistory": [1, 2, 3, 4], "indexForward": [], "setPageHistory": [1, 2, 3, 4], "children": [undefined, undefined, undefined, undefined, undefined, undefined]});

            pageStack.showPage(5);
            compare(pageStack.currentPage.title, "App settings", "app settings page shown");
            compare(drawer.currentItem.name, "App settings", "drawer index updated for app settings");
            compare(toolBar.currentIndex, 4, "tab bar index not changed for app settings");
            compare(pageStack.serialize(), {"index": 5, "indexHistory": [1, 2, 3, 4, 5], "indexForward": [], "setPageHistory": [1, 2, 3, 4, 5], "children": [undefined, undefined, undefined, undefined, undefined, undefined]});

            pageStack.pop();
            compare(pageStack.currentPage.title, "Advanced", "can go back to previous page");
            compare(drawer.currentItem.name, "Advanced", "drawer index updated when going back");
            compare(toolBar.currentIndex, 4, "tab bar index unchanged when going back from app to advanced settings");
            compare(pageStack.serialize(), {"index": 4, "indexHistory": [1, 2, 3, 4], "indexForward": [5], "setPageHistory": [1, 2, 3, 4], "children": [undefined, undefined, undefined, undefined, undefined, undefined]});

            pageStack.pop();
            compare(pageStack.currentPage.title, "Recent changes", "can go back further");
            compare(drawer.currentItem.name, "Recent changes", "drawer index updated when going back further");
            compare(toolBar.currentIndex, 3, "tab bar index updated when going back further to recent changes");
            compare(pageStack.serialize(), {"index": 3, "indexHistory": [1, 2, 3], "indexForward": [5, 4], "setPageHistory": [1, 2, 3], "children": [undefined, undefined, undefined, undefined, undefined, undefined]});

            pageStack.forward();
            compare(pageStack.currentPage.title, "Advanced", "can go forward again");
            compare(drawer.currentItem.name, "Advanced", "drawer index updated when going forward");
            compare(toolBar.currentIndex, 4, "tab bar index updated when going forward");
            compare(pageStack.serialize(), {"index": 4, "indexHistory": [1, 2, 3, 4], "indexForward": [5], "setPageHistory": [1, 2, 3], "children": [undefined, undefined, undefined, undefined, undefined, undefined]});

            pageStack.forward();
            compare(pageStack.currentPage.title, "App settings", "can go forward again (2)");
            compare(drawer.currentItem.name, "App settings", "drawer index updated when going forward (2)");
            compare(pageStack.serialize(), {"index": 5, "indexHistory": [1, 2, 3, 4, 5], "indexForward": [], "setPageHistory": [1, 2, 3], "children": [undefined, undefined, undefined, undefined, undefined, undefined]});
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
                foldersAdded += 1;

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
                tryVerify(() => model.rowCount() === foldersAdded, 5000, "new folder present");
                const rowCount = model.rowCount();
                const firstFolderIndex = model.index(rowCount - 1, 0);
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

        function test_advancedPage() {
            if (!withSyncthing) {
                skip("not testing advanced page without having Syncthing started");
                return;
            }
            pageStack.setCurrentIndex(4);

            const advancedPage = pageStack.currentPage;
            compare(advancedPage.title, "Advanced", "advanced page shown");
            compare(advancedPage.isDangerous, false, "no dangerous settings made yet");
            compare(pageStack.currentActions.map(a => a.enabled), [false, false], "no actions enabled without changes");

            const listView = advancedPage.listView;
            const model = listView.model;
            tryCompare(model, "count", 9, 5000, "advanced settings shown");
            compare(model.get(optionsIndex).label, "Various options");
            listView.currentIndex = optionsIndex;
            listView.currentItem.click();

            const optionsPage = pageStack.currentPage;
            const optionsModel = optionsPage.model;
            const optionsListView = optionsPage.listView;
            compare(optionsPage.title, "Various advanced options", "options page shown");
            compare(optionsPage.isDangerous, true, "options considered dangerious");
            tryVerify(() => optionsModel.count >= 56, 5000, "options shown");
            compare(optionsModel.get(7).label, "Local Discovery");
            compare(optionsModel.get(7).value, true, "local discovery enabled by default");
            compare(optionsModel.get(8).label, "Global Discovery");
            compare(optionsModel.get(8).value, true, "global discovery enabled by default");
            const state = pageStack.serialize();
            compare(state.index, 4);
            const settingsState = state?.children[4];
            compare(settingsState.componentName, "ObjectConfigPage.qml", "component name serialized");
            compare(settingsState.path, "options", "path serialized");
            compare(settingsState.topLevelObject.options.localAnnounceEnabled, true, "local discovery initially enabled");
            optionsListView.currentIndex = localDiscoveryIndex;
            optionsListView.currentItem.click();
            tryVerify(() => optionsModel.get(localDiscoveryIndex).value === false, 5000, "local discovery unselected");

            pageStack.pop();
            tryCompare(advancedPage, "title", "Advanced - changes not saved yet", 5000, "pending changes shown in title");
            compare(advancedPage.isDangerous, true, "dangerous setting has been made yet");
            compare(pageStack.currentActions.map(a => a.enabled), [true, true], "actions enabled with pending changes");
            compare(listView.currentIndex, optionsIndex, "options still selected");
            listView.currentItem.click();

            const optionsPage2 = pageStack.currentPage;
            const optionsModel2 = optionsPage2.model;
            compare(optionsModel2.get(localDiscoveryIndex).label, "Local Discovery");
            compare(optionsModel2.get(localDiscoveryIndex).value, false, "local discovery still showing as disabled");
            const state2 = pageStack.serialize();
            compare(state2.index, 4);
            const settingsState2 = state2?.children[4];
            compare(settingsState2.topLevelObject.options.localAnnounceEnabled, false, "pending change serialized");

            pageStack.pop();
            compare(advancedPage.isDangerous, true, "dangerous setting are still pending");
            const applyAction = pageStack.currentActions[1];
            compare(applyAction.text, "Apply changes");
            applyAction.trigger();
            wait(1); // saving might be triggered asynchronously
            tryVerify(() => !App.savingConfig);

            compare(advancedPage.title, "Advanced", "title changed back after all settings have been saved");
            compare(advancedPage.isDangerous, false, "no dangerous settings pending anymore");
            compare(listView.currentIndex, optionsIndex, "options still selected after saving");
            listView.currentItem.click();

            const optionsPage3 = pageStack.currentPage;
            const optionsModel3 = optionsPage3.model;
            const optionsListView3 = optionsPage3.listView;
            compare(optionsModel3.get(localDiscoveryIndex).label, "Local Discovery");
            compare(optionsModel3.get(localDiscoveryIndex).value, false, "local discovery still showing as disabled after saving");
            compare(optionsModel3.get(globalDiscoveryIndex).label, "Global Discovery");
            compare(optionsModel3.get(globalDiscoveryIndex).value, true, "global discovery still enabled after saving");

            optionsListView3.currentIndex = globalDiscoveryIndex;
            optionsListView3.currentItem.click();
            tryVerify(() => optionsModel3.get(globalDiscoveryIndex).value === false, 5000, "global discovery unselected");
            pageStack.pop();
            compare(advancedPage.title, "Advanced - changes not saved yet", "title changed again after disabling global discovery");
            compare(advancedPage.isDangerous, true, "dangerous setting has been made again");
            const discardAction = pageStack.currentActions[0];
            compare(discardAction.text, "Discard changes");
            discardAction.trigger();
            tryCompare(advancedPage, "title", "Advanced", 5000, "title changed back after all settings have been discarded");
            tryCompare(advancedPage, "isDangerous", false, 5000, "dangerous setting discarded");

            compare(listView.currentIndex, optionsIndex, "options still selected after saving");
            listView.currentItem.click();

            const optionsPage4 = pageStack.currentPage;
            const optionsModel4 = optionsPage4.model;
            compare(optionsModel4.get(localDiscoveryIndex).label, "Local Discovery");
            compare(optionsModel4.get(localDiscoveryIndex).value, false, "local discovery still showing as disabled after discarding");
            compare(optionsModel4.get(globalDiscoveryIndex).label, "Global Discovery");
            compare(optionsModel4.get(globalDiscoveryIndex).value, true, "global discovery still enabled after discarding");
            pageStack.pop();

            goBackToStartPage();
        }

        function test_settingsPage() {
            if (!withSyncthing) {
                skip("not testing settings and import without having Syncthing started");
                return;
            }
            pageStack.setCurrentIndex(5);

            const settingsPage = pageStack.currentPage;
            compare(settingsPage.title, "App settings", "settings page shown");
            compare(settingsPage.isDangerous, undefined, "top-level app settings not dangerous");

            const listView = settingsPage.listView;
            const model = listView.model;
            tryCompare(model, "count", 12, 5000, "app settings shown");
            compare(model.get(importIndex).label, "Import selected settings/secrets/data of app and backend");

            // trigger import
            listView.currentIndex = importIndex;
            const folderDlg = settingsPage.backupFolderDialog;
            folderDlg.popupType = Popup.Item;
            folderDlg.options |= FolderDialog.DontUseNativeDialog;
            listView.currentItem.click();
            tryCompare(folderDlg, "visible", true, 5000, "folder dialog shown");
            folderDlg.selectedFolder = testConfigDir;
            folderDlg.accept();
            tryVerify(() => pageStack.currentPage.title.includes("import"), 5000, "import page shown");

            const importPage = pageStack.currentPage;
            compare(importPage.title, "Select settings to import", "on import page");
            compare(importPage.isDangerous, false, "no dangerous option selected by default");
            compare(importPage.folderSelection.selectionEnabled, false, "no folders selected by default");
            compare(importPage.deviceSelection.selectionEnabled, false, "no devices selected by default");

            const folderSelection = importPage.folderSelection;
            const folderSelectionModel = folderSelection.model;
            const folderSelectionView = folderSelection.selectionView;
            folderSelection.click();
            tryCompare(folderSelectionView, "visible", true, 5000, "folder selection shown");
            compare(folderSelectionModel.count, 2, "two folders available for import");
            compare(folderSelectionModel.get(0).displayName, "test1", "first folder shown");
            verify(folderSelectionModel.get(0).path.includes("some/path/1"), "first folder path shown");
            compare(folderSelectionModel.get(0).checked, false, "first folder not selected");
            compare(folderSelectionModel.get(1).displayName, "Test dir 2", "second folder shown");
            verify(folderSelectionModel.get(1).path.includes("some/path/2"), "second folder path shown");
            compare(folderSelectionModel.get(1).checked, false, "second folder not selected");
            folderSelectionView.currentIndex = 1;
            folderSelectionView.currentItem.click();
            compare(folderSelectionModel.get(0).checked, false, "first folder still not selected");
            compare(folderSelectionModel.get(1).checked, true, "second folder selected");
            folderSelection.accept();
            compare(folderSelection.selectionEnabled, true, "folders have been selected");

            const importAction = pageStack.currentActions[0];
            compare(importAction.text, "Import selected");
            importAction.trigger();
            foldersAdded += 1;

            // wait until the import is done
            wait(1); // import might be triggered asynchronously
            tryVerify(() => !App.isImportExportOngoing);
            const importMessage = `Merging 1 folders and 0 devices`;
            let importMessageIndex = 0;
            tryVerify(() => (importMessageIndex = messages.indexOf(importMessage) >= 0), 5000, `received notification "${importMessage}"`);
            tryVerify(() => messages.indexOf("App settings saved", importMessageIndex) >= 0, 5000, "received notification about settings saved");
            tryVerify(() => messages.indexOf("Configuration changed", importMessageIndex) >= 0, 5000, "received notification about config changed");
            pageStack.pop();

            // check whether imported folder shows up
            pageStack.setCurrentIndex(1);
            const foldersPage = pageStack.currentPage;
            const folderModel = foldersPage.model;
            compare(foldersPage.title, "Folders", "on folders page");
            tryVerify(() => folderModel.rowCount() === foldersAdded, 5000, "newly imported folder present");
            const firstFolderIndex = folderModel.index(0, 0);
            compare(folderModel.data(firstFolderIndex, directoryIdRole), "test2", "new folder ID present");
            verify(folderModel.data(firstFolderIndex, directoryPathRole).includes("some/path/2"), "new folder path present");
            pageStack.pop();

            // trigger export
            compare(settingsPage.title, "App settings", "back on settings page");
            const listView2 = settingsPage.listView;
            compare(listView2.model.get(exportIndex).label, "Export all settings/secrets/data of app and backend");
            listView2.currentIndex = exportIndex;
            listView2.currentItem.click();
            const folderDlg2 = settingsPage.backupFolderDialog;
            tryCompare(folderDlg2, "visible", true, 5000, "folder dialog shown");
            folderDlg2.selectedFolder = testExportDir;
            folderDlg2.accept();

            // wait until the export is done
            const exportMessage = `Settings have been exported to "${testExportDir}".`;
            tryVerify(() => messages.indexOf(exportMessage) >= 0, 10000, `received notification "${exportMessage}"`);
            verify(messages.indexOf("Another import/export still pending") < 0, "no multiple attempts to trigger an import/export was made");

            setup.checkExport();

            goBackToStartPage();
        }
    }
}
