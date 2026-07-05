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
    onClearButtonPressed: field.clear()
    Keys.onEscapePressed: field.clear()
    required property ExpandableListView view
    function toggle() {
        return (field.visible = !field.visible) && field.forceActiveFocus();
    }
    function clear() {
        view.mainModel?.setFilterRegularExpressionPattern("");
        field.text = "";
        field.visible = false;
    }
}
