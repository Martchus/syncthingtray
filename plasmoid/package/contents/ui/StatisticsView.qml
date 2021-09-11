import QtQuick 2.7
import QtQuick.Layouts 1.1

RowLayout {
    id: rowLayout
    property var statistics
    property string context: "?"

    IconLabel {
        iconSource: plasmoid.nativeInterface.loadForkAwesomeIcon("file")
        text: statistics.files !== undefined ? statistics.files : "?"
        tooltip: context + qsTr(" files")
    }
    IconLabel {
        iconSource: plasmoid.nativeInterface.loadForkAwesomeIcon("folder")
        text: statistics.dirs !== undefined ? statistics.dirs : "?"
        tooltip: context + qsTr(" directories")
    }
    IconLabel {
        iconSource: plasmoid.nativeInterface.loadForkAwesomeIcon("hdd")
        text: statistics.bytes !== undefined ? plasmoid.nativeInterface.formatFileSize(
                                                   statistics.bytes) : "?"
        tooltip: context + qsTr(" size")
    }
}
