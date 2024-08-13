import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Dialog {
    id: aboutDialog
    modal: true
    focus: true
    parent: null
    anchors.centerIn: parent
    standardButtons: Dialog.Ok
    width: 300
    title: qsTr("About")
    contentItem: ColumnLayout {
        Image {
            readonly property double size: 128
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: size
            Layout.preferredHeight: size
            source: "qrc:/icons/hicolor/scalable/app/syncthingtray.svg"
            sourceSize.width: size
            sourceSize.height: size
        }
        Label {
            text: Qt.application.name
            Layout.alignment: Qt.AlignHCenter
        }
        Label {
            text: "Version " + Qt.application.version
            Layout.alignment: Qt.AlignHCenter
        }
        Label {
            text: "Developed by " + Qt.application.organization
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
