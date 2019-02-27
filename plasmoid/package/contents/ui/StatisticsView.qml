import QtQuick 2.7
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

RowLayout {
    property var statistics

    PlasmaCore.IconItem {
        Layout.preferredWidth: 16
        Layout.preferredHeight: 16
        source: plasmoid.nativeInterface.loadFontAwesomeIcon("file", false)
        opacity: 0.7
    }
    PlasmaComponents.Label {
        text: statistics.files !== undefined ? statistics.files : "?"
    }
    PlasmaCore.IconItem {
        Layout.preferredWidth: 16
        Layout.preferredHeight: 16
        source: plasmoid.nativeInterface.loadFontAwesomeIcon("folder", false)
        opacity: 0.7
    }
    PlasmaComponents.Label {
        text: statistics.dirs !== undefined ? statistics.dirs : "?"
    }
    PlasmaCore.IconItem {
        Layout.preferredWidth: 16
        Layout.preferredHeight: 16
        source: plasmoid.nativeInterface.loadFontAwesomeIcon("hdd", false)
        opacity: 0.7
    }
    PlasmaComponents.Label {
        text: statistics.bytes !== undefined ? plasmoid.nativeInterface.formatFileSize(
                                                   statistics.bytes) : "?"
    }
}
