import QtQuick
import QtQuick.Controls

CustomListView {
    id: mainView
    anchors.fill: parent
    model: ExpandableDelegate {
        mainView: mainView
    }
    required property QtObject mainModel
    property StackView stackView
}
