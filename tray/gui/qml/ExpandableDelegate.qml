import QtQuick

DelegateModel {
    id: mainDelegateModel
    model: mainView.mainModel
    delegate: ExpandableItemDelegate {
        mainView: mainDelegateModel.mainView
    }

    required property ListView mainView
}
