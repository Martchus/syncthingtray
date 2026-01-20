import QtQuick
import QtQuick.Layouts

import Main

Image {
    Layout.preferredWidth: size
    Layout.preferredHeight: size
    Layout.maximumWidth: size
    source: `${QuickUI.faUrlBase}${iconName}::${QuickUI.darkmodeEnabled}`
    width: size
    height: size
    property int size: 16
    required property string iconName
}
