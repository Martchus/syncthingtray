import QtQuick
import QtQuick.Controls.Material

DelegateModel {
    id: mainDelegateModel
    model: mainView.model
    delegate: ExpandableItemDelegate {
        mainView: mainDelegateModel.mainView
    }

    required property ListView mainView
    property StackView stackView
}
