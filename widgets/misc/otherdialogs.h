#ifndef SYNCTHINGWIDGETS_OTHERDIALOGS_H
#define SYNCTHINGWIDGETS_OTHERDIALOGS_H

#include "../global.h"

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QDialog)

namespace Data {
class SyncthingConnection;
}

namespace QtGui {

SYNCTHINGWIDGETS_EXPORT QDialog *ownDeviceIdDialog(Data::SyncthingConnection &connection);
}

#endif // SYNCTHINGWIDGETS_OTHERDIALOGS_H
