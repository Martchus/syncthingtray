import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material

import Main

StackView {
    id: stackView
    Layout.fillWidth: true
    Layout.fillHeight: true
    initialItem: AdvancedMainPage {
        id: advancedPage
        stackView: stackView
    }
    function showGuiAuth() {
        advancedPage.hasUnsavedChanges = true;
        advancedPage.listView.currentIndex = 0;
        advancedPage.listView.currentItem.click();
    }
    required property var pages
}
