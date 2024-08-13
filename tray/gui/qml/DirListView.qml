import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ExpandableListView {
    id: mainView
    model: DirDelegate {
        mainView: mainView
    }
}
