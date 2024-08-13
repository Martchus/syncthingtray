import QtQuick
import QtQuick.Controls

ListView {
    id: mainView
    anchors.fill: parent
    model: ExpandableDelegate {
        mainView: mainView
    }
    ScrollIndicator.vertical: ScrollIndicator { }

    required property QtObject mainModel
}
