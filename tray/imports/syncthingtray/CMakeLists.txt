set_source_files_properties(Constants.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE true
)

set_source_files_properties(AppSettings.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE true
)

qt_add_library(${META_PROJECT_VARNAME} STATIC)
qt_add_qml_module(${META_PROJECT_VARNAME}
    URI "${META_PROJECT_VARNAME}"
    VERSION 1.0
    QML_FILES
        AppSettings.qml
        Constants.qml
        RoomsModel.qml
)
