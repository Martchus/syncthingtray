#ifndef SETTINGS_WIZARD_ENUMS_H
#define SETTINGS_WIZARD_ENUMS_H

#include <c++utilities/misc/flagenumclass.h>

#include <QtGlobal>

namespace QtGui {

enum class MainConfiguration : quint64 {
    None,
    CurrentlyRunning,
    LauncherExternal,
    LauncherBuiltIn,
    SystemdUserUnit,
    SystemdSystemUnit,
};

enum class ExtraConfiguration : quint64 {
    None,
    SystemdIntegration = (1 << 0),
};

} // namespace QtGui

CPP_UTILITIES_MARK_FLAG_ENUM_CLASS(QtGui, QtGui::ExtraConfiguration)

#endif // SETTINGS_WIZARD_H
