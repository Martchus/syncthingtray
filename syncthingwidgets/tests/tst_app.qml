import QtQuick
import QtQuick.Layouts
import QtTest
import Main


Item {
    LeftDrawer {
        id: drawer
        pageStack: pageStack
        aboutDialog: aboutDialog
        closeDialog: closeDialog
    }
    ColumnLayout {
        anchors.fill: parent
        MainToolBar {
            drawer: drawer
            pageStack: pageStack
        }
        PageStack {
            id: pageStack
            window: testCase
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

        function cleanup() {
            pageStack.showPage(0);
        }

        function test_miscProperties() {
            compare(meta.title, "Syncthing", "title set");
            verify(theming.font, "font set");
        }

        function test_pageSwitching() {
            compare(pageStack.currentPage.title, "Syncthing", "start page shown initially");
            compare(drawer.currentItem.name, "Start", "drawer index initialized for start page");

            pageStack.showPage(1);
            compare(pageStack.currentPage.title, "Folders", "folders page shown");
            compare(drawer.currentItem.name, "Folders", "drawer index updated for folders page");

            pageStack.showPage(2);
            compare(pageStack.currentPage.title, "Devices", "devices page shown");
            compare(drawer.currentItem.name, "Devices", "drawer index updated for devices page");

            pageStack.showPage(3);
            compare(pageStack.currentPage.title, "Recent changes", "changes page shown");
            compare(drawer.currentItem.name, "Recent changes", "drawer index updated for changes page");

            pageStack.showPage(4);
            compare(pageStack.currentPage.title, "Advanced", "advanced page shown");
            compare(drawer.currentItem.name, "Advanced", "drawer index updated for advanced page");

            pageStack.showPage(5);
            compare(pageStack.currentPage.title, "App settings", "app settings page shown");
            compare(drawer.currentItem.name, "App settings", "drawer index updated for app settings");

            pageStack.pop();
            compare(pageStack.currentPage.title, "Advanced", "can go back to previous page");
            compare(drawer.currentItem.name, "Advanced", "drawer index updated when going back");

            pageStack.pop();
            compare(pageStack.currentPage.title, "Recent changes", "can go back further");
            compare(drawer.currentItem.name, "Recent changes", "drawer index updated when going back further");

            pageStack.forward();
            compare(pageStack.currentPage.title, "Advanced", "can go forward again");
            compare(drawer.currentItem.name, "Advanced", "drawer index updated when going forward");

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
            const folderModel = addFolderPage.model;
            verify(folderModel.count > 0, "model populated");
            compare(folderModel.get(0).label, "Paused", "paused label present");
            compare(folderModel.get(0).value, true, "paused enabled by default");
            compare(drawer.currentItem.name, "Folders", "drawer index updated to folders page");

            pageStack.pop();
            compare(pageStack.currentPage.title, "Syncthing", "back on start page");
            compare(drawer.currentItem.name, "Start", "drawer index updated to start page");
        }
    }
}
