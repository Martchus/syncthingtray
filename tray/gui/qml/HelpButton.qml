import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RoundButton {
    visible: configCategory.length > 0
    hoverEnabled: true
    Layout.preferredWidth: 36
    Layout.preferredHeight: 36
    ToolTip.visible: hovered || pressed
    ToolTip.text: qsTr("Open help")
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
    icon.source: app.faUrlBase + "question"
    icon.width: 20
    icon.height: 20
    onClicked: Qt.openUrlExternally(`https://docs.syncthing.net/users/config#${configCategory}.${key.toLowerCase()}`)
    property string configCategory
    required property string key
}
