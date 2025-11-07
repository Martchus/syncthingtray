#ifndef SYNCTHINGWIDGETS_UTILS_H
#define SYNCTHINGWIDGETS_UTILS_H

#include "../global.h"

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QString)

namespace Data {
class SyncthingConnection;
}

namespace QtGui {

SYNCTHINGWIDGETS_EXPORT void handleRelevantControlsChanged(bool visible, int tabIndex, Data::SyncthingConnection &connection);
SYNCTHINGWIDGETS_EXPORT QString readmeUrl();
SYNCTHINGWIDGETS_EXPORT QString documentationUrl();

} // namespace QtGui

#endif // SYNCTHINGWIDGETS_UTILS_H
