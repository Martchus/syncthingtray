/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import app

Button {
    id: root

    required property string name
    required property bool isSmallLayout
    required property bool isEnabled
    required property bool isActive

    leftPadding: 0
    rightPadding: 0
    topPadding: root.isSmallLayout ? 7 : 12
    bottomPadding: root.isSmallLayout ? 7 : 20
    spacing: root.isSmallLayout ? 5 : 10

    enabled: root.isEnabled
    checked: root.isActive && root.isEnabled
    checkable: true
    flat: true
    autoExclusive: true
    display: AbstractButton.TextUnderIcon

    text: root.name
    icon.source: "images/" + root.name + ".svg"
    icon.width: root.isSmallLayout ? 20 : 32
    icon.height: root.isSmallLayout ? 20 : 32

    palette.brightText: "#2CDE85"
    palette.dark: "transparent"
    palette.windowText: root.isEnabled ? Constants.accentTextColor : "#898989"

    font.family: "Titillium Web"
    font.pixelSize: !root.isSmallLayout ? 12 : 10
    font.weight: 600

    Image {
        anchors.topMargin: 2
        anchors.top: root.top
        anchors.horizontalCenter: root.horizontalCenter
        source: "images/circle.svg"
        visible: (root.down || root.checked) && !root.isSmallLayout
    }
}
