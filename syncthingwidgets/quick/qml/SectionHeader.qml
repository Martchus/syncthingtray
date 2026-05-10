import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

RowLayout {
    id: sectionHeader
    spacing: 10
    width: parent.width
    SectionLabel {
        text: sectionHeader.section
    }
    required property string section
}
