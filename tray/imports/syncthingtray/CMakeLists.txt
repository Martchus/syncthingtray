set_source_files_properties(Constants.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE true
)

set_source_files_properties(AppSettings.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE true
)

qt_add_library(${META_PROJECT_VARNAME}App STATIC)
qt_add_qml_module(${META_PROJECT_VARNAME}App
    URI "${META_PROJECT_VARNAME}App"
    VERSION 1.0
    QML_FILES
        AppSettings.qml
        Constants.qml
        RoomsModel.qml
)
