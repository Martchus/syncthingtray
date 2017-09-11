import QtQuick 2.7
import QtQuick.Layouts 1.1
import org.kde.plasma.components 2.0 as PlasmaComponents

RowLayout {
    PlasmaComponents.Label {
        Layout.preferredWidth: 100
        Layout.leftMargin: units.iconSizes.smallMedium
        text: name
        font.pointSize: theme.defaultFont.pointSize * 0.8
        elide: Text.ElideRight
    }
    PlasmaComponents.Label {
        Layout.fillWidth: true
        text: detail
        font.pointSize: theme.defaultFont.pointSize * 0.8
        elide: Text.ElideRight
    }
}
