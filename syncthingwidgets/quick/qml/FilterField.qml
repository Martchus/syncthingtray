import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main
import Tray

SearchField {
    id: field
    Layout.fillWidth: true
    Layout.leftMargin: 5
    Layout.rightMargin: view.ScrollBar?.vertical.width
    visible: false
    width: 250
    live: true
    onSearchTriggered: view.mainModel?.setFilterRegularExpressionPattern(text)
    onClearButtonPressed: {
        view.mainModel?.setFilterRegularExpressionPattern("");
        field.visible = false;
    }
    required property ExpandableListView view
    function toggle() {
        return (field.visible = !field.visible) && field.forceActiveFocus();
    }
}
