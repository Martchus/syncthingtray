#ifndef DATA_UTILS_H
#define DATA_UTILS_H

#include "./global.h"

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QUrl)

namespace ChronoUtilities {
class DateTime;
}

namespace Data {

QString LIB_SYNCTHING_CONNECTOR_EXPORT agoString(ChronoUtilities::DateTime dateTime);
bool LIB_SYNCTHING_CONNECTOR_EXPORT isLocal(const QUrl &url);

}

#endif // DATA_UTILS_H
