import QtQuick 2.7
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

RowLayout {
    id: rowLayout
    property var statistics
    property string context: "?"

    IconLabel {
        iconSource: plasmoid.nativeInterface.loadFontAwesomeIcon("file", false)
        text: statistics.files !== undefined ? statistics.files : "?"
        tooltip: context + qsTr(" files")
    }
    IconLabel {
        iconSource: plasmoid.nativeInterface.loadFontAwesomeIcon("folder", false)
        text: statistics.dirs !== undefined ? statistics.dirs : "?"
        tooltip: context + qsTr(" directories")
    }
    IconLabel {
        iconSource: plasmoid.nativeInterface.loadFontAwesomeIcon("hdd", false)
        text: statistics.bytes !== undefined ? plasmoid.nativeInterface.formatFileSize(
                                                   statistics.bytes) : "?"
        tooltip: context + qsTr(" size")
    }
}
