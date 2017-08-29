#ifndef SYNCTHINGWIDGETS_OTHERDIALOGS_H
#define SYNCTHINGWIDGETS_OTHERDIALOGS_H

#include "../global.h"

#include <QWidget>

namespace Data {
class SyncthingConnection;
}

namespace QtGui {

QWidget SYNCTHINGWIDGETS_EXPORT *ownDeviceIdDialog(Data::SyncthingConnection &connection);
}

#endif // SYNCTHINGWIDGETS_OTHERDIALOGS_H
