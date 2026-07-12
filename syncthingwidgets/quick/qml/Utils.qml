pragma Singleton

import QtQml
import QtQuick.Controls.Material

import Main

QtObject {
    readonly property bool flatDialogButtons: QuickUI.style === "Material"
    readonly property bool winUI: QuickUI.style === "FluentWinUI3"
    readonly property var popupType: {
        switch (QuickUI.popupType) {
        case 1:
            return Popup.Window;
        case 2:
            return Popup.Native;
        default:
            return Popup.Item;
        }
    }

    function submitPage(page, force) {
        // handle "dangerous flag" and unsaved changes
        if (!page.hasUnsavedChanges || force) {
            return false;
        }
        const parentPage = page.parentPage;
        if (parentPage?.hasUnsavedChanges === undefined) {
            return true;
        }
        parentPage.hasUnsavedChanges = true;
        page.hasUnsavedChanges = false;
        if (page.isDangerous) {
            // propagate "dangerous flag" to parent page if changes on a dangerous page were made
            parentPage.isDangerous = true;
        }
        return false;
    }
}
