#ifndef DATA_SYNCTHINGICONS_H
#define DATA_SYNCTHINGICONS_H

#include "./global.h"

#include <QIcon>
#include <QSize>

namespace Data {

QPixmap LIB_SYNCTHING_MODEL_EXPORT renderSvgImage(const QString &path, const QSize &size = QSize(128, 128));

struct StatusIcons {
    StatusIcons();
    QIcon disconnected;
    QIcon idling;
    QIcon scanninig;
    QIcon notify;
    QIcon pause;
    QIcon sync;
    QIcon syncComplete;
    QIcon error;
    QIcon errorSync;
    QIcon newItem;
};

const StatusIcons LIB_SYNCTHING_MODEL_EXPORT &statusIcons();

} // namespace Data

#endif // DATA_SYNCTHINGICONS_H
