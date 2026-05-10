import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

// This is a version of SectionHeader that does not
// use `required property string section` as this seems to break if `qmlcachegen`
// is used. Using `required property var section` has the same problem and using
// `property string section` does not work at all.

// This version is only used in list views and can therefore use `ListView.view`
// instead of `parent` which might also be better.

RowLayout {
    spacing: 10
    width: ListView.view.width
    SectionLabel {
        text: section // read section from context as required property breaks with `qmlcachegen`
    }
}
