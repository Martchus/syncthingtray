import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

ExpandableListView {
    id: mainView
    model: DirDelegate {
        mainView: mainView
    }
}
