#ifndef SYNCTHINGWIDGETS_OTHERDIALOGS_H
#define SYNCTHINGWIDGETS_OTHERDIALOGS_H

#include "../global.h"

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QDialog)
QT_FORWARD_DECLARE_CLASS(QWidget)

namespace Data {
class SyncthingConnection;
}

namespace QtGui {

SYNCTHINGWIDGETS_EXPORT QDialog *ownDeviceIdDialog(Data::SyncthingConnection &connection);
SYNCTHINGWIDGETS_EXPORT QWidget *ownDeviceIdWidget(Data::SyncthingConnection &connection, int size, QWidget *parent = nullptr);
} // namespace QtGui

#endif // SYNCTHINGWIDGETS_OTHERDIALOGS_H
