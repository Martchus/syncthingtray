import QtQuick
import QtQuick.Controls

DelegateModel {
    id: mainDelegateModel
    model: mainView.mainModel
    delegate: ExpandableItemDelegate {
        mainView: mainDelegateModel.mainView
    }

    required property ListView mainView
    property StackView stackView
}
