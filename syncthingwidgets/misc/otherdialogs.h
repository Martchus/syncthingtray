#ifndef SYNCTHINGWIDGETS_OTHERDIALOGS_H
#define SYNCTHINGWIDGETS_OTHERDIALOGS_H

#include "../global.h"

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QDialog)
QT_FORWARD_DECLARE_CLASS(QWidget)

namespace Data {
class SyncthingConnection;
struct SyncthingDir;
} // namespace Data

namespace QtGui {
class TextViewDialog;

SYNCTHINGWIDGETS_EXPORT QDialog *ownDeviceIdDialog(Data::SyncthingConnection &connection);
SYNCTHINGWIDGETS_EXPORT QWidget *ownDeviceIdWidget(Data::SyncthingConnection &connection, int size, QWidget *parent = nullptr);
SYNCTHINGWIDGETS_EXPORT QDialog *browseRemoteFilesDialog(
    Data::SyncthingConnection &connection, const Data::SyncthingDir &dir, QWidget *parent = nullptr);
SYNCTHINGWIDGETS_EXPORT QDialog *errorNotificationsDialog(Data::SyncthingConnection &connection, QWidget *parent = nullptr);
SYNCTHINGWIDGETS_EXPORT TextViewDialog *ignorePatternsDialog(
    Data::SyncthingConnection &connection, const Data::SyncthingDir &dir, QWidget *parent = nullptr);

} // namespace QtGui

#endif // SYNCTHINGWIDGETS_OTHERDIALOGS_H
