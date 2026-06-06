pragma Singleton

import QtQml
import QtQuick.Controls.Material

import Main

QtObject {
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
}
