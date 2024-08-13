import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Main

ListView {
    id: detailsView
    Layout.fillWidth: true
    Layout.leftMargin: 24
    Layout.rightMargin: 6
    Layout.bottomMargin: 6
    visible: false

    required property ListView mainView

    onCountChanged: {
        var d = delegate.createObject(detailsView, {detailName: "", detailValue: ""});
        height = count * d.height
        d.destroy()
    }

    model: DelegateModel {
        model: mainView.mainModel
        rootIndex: mainView.mainModel.index(modelData.index, 0)
        delegate: Item {
            id: detailItem

            property string detailName: name ? name : ""
            property string detailValue: detail ? detail : ""

            width: detailRow.implicitWidth
            height: detailRow.implicitHeight

            RowLayout {
                id: detailRow
                width: parent.width

                Icon {
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16
                    source: detailIcon
                    width: 16
                    height: 16
                    opacity: 0.8
                }
                Label {
                    Layout.preferredWidth: 100
                    text: detailName
                    font.weight: Font.DemiBold
                }
                Label {
                    Layout.fillWidth: true
                    text: detailValue
                    elide: Text.ElideRight
                    horizontalAlignment: Qt.AlignRight
                }
            }
        }
    }
}
