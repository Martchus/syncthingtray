import QtQuick 2.3
import QtQuick.Layouts 1.1
import org.kde.plasma.components 3.0 as PlasmaComponents3

RowLayout {
    spacing: 10
    width: ListView.view.width
    PlasmaComponents3.Label {
        Layout.fillWidth: true
        Layout.topMargin: 10
        Layout.bottomMargin: 3
        text: section // from context, `required property string section` might not work, see DynamicSectionHeader
        elide: Text.ElideRight
        font.weight: Font.Medium
        wrapMode: Text.WordWrap
    }
}
