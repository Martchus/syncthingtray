import QtQuick 2.7
import QtQuick.Layouts 1.1

RowLayout {
    id: rowLayout
    property var statistics
    property string context: "?"

    IconLabel {
        iconSource: plasmoid.faUrl + "file-o"
        text: statistics.files !== undefined ? statistics.files : "?"
        tooltip: context + qsTr(" files")
    }
    IconLabel {
        iconSource: plasmoid.faUrl + "folder-o"
        text: statistics.dirs !== undefined ? statistics.dirs : "?"
        tooltip: context + qsTr(" directories")
    }
    IconLabel {
        iconSource: plasmoid.faUrl + "hdd-o"
        text: statistics.bytes !== undefined ? plasmoid.formatFileSize(
                                                   statistics.bytes) : "?"
        tooltip: context + qsTr(" size")
    }
}
