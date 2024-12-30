import QtQuick
import QtQuick.Layouts

import Main

Image {
    Layout.preferredWidth: size
    Layout.preferredHeight: size
    Layout.maximumWidth: size
    source: `${App.faUrlBase}${iconName}::${App.darkmodeEnabled}`
    width: size
    height: size
    property int size: 16
    required property string iconName
}
