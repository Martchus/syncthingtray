#ifndef SYNCTHINGWIDGETS_UTILS_H
#define SYNCTHINGWIDGETS_UTILS_H

#include "../global.h"

#include <c++utilities/misc/flagenumclass.h>

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QString)

namespace Data {
class SyncthingConnection;
}

namespace QtGui {

enum class VisibleControls {
    None = 0,
    TrayWidget = (1 << 0),
    MainWindow = (1 << 1),
    RecentChangesWindow = (1 << 2),
};

SYNCTHINGWIDGETS_EXPORT void handleRelevantControlsChanged(VisibleControls visibleControls, int tabIndex, Data::SyncthingConnection &connection);
SYNCTHINGWIDGETS_EXPORT QString readmeUrl();
SYNCTHINGWIDGETS_EXPORT QString documentationUrl();

} // namespace QtGui

CPP_UTILITIES_MARK_FLAG_ENUM_CLASS(QtGui, QtGui::VisibleControls)

#endif // SYNCTHINGWIDGETS_UTILS_H
