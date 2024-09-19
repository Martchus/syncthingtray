import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Main

ListView {
    id: detailsView
    Layout.fillWidth: true
    visible: false
    interactive: false

    required property ItemDelegate mainDelegate
    property ListView mainView: mainDelegate.mainView

    onCountChanged: {
        var d = delegate.createObject(detailsView, {detailName: "", detailValue: ""});
        height = count * d.height
        d.destroy()
    }

    model: DelegateModel {
        model: mainView.mainModel
        rootIndex: mainView.mainModel?.index(modelData.index, 0)
        delegate: Item {
            id: detailItem
            width: detailsView.width
            height: detailRow.implicitHeight

            property string detailName: name ?? ""
            property string detailValue: detail ?? ""

            RowLayout {
                id: detailRow
                width: parent.width
                spacing: 10
                Item {
                    Layout.preferredWidth: mainDelegate.statusIconWidth
                    Layout.preferredHeight: 18
                    Icon {
                        anchors.centerIn: parent
                        source: detailIcon
                        width: 16
                        height: 16
                    }
                }
                Label {
                    text: detailName
                    font.weight: Font.Light
                }
                Label {
                    Layout.fillWidth: true
                    text: detailValue
                    elide: Text.ElideRight
                    font.weight: Font.Light
                    horizontalAlignment: Qt.AlignRight
                    MouseArea {
                        anchors.fill: parent
                        onPressAndHold: app.copyText(detailValue)
                    }
                }
            }
        }
    }
}
