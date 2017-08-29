import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras

RowLayout {
    Item {
        width: 21
    }
    PlasmaComponents.Label {
        Layout.preferredWidth: 100
        text: name
        font.pointSize: 8
        height: contentHeight
        elide: Text.ElideRight
    }
    PlasmaComponents.Label {
        Layout.fillWidth: true
        text: detail
        font.pointSize: 8
        height: contentHeight
        elide: Text.ElideRight
    }
}
