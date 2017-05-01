#ifndef DATA_UTILS_H
#define DATA_UTILS_H

#include "./global.h"

#include <QStringList>

QT_FORWARD_DECLARE_CLASS(QUrl)

namespace ChronoUtilities {
class DateTime;
}

namespace Data {

QString LIB_SYNCTHING_CONNECTOR_EXPORT agoString(ChronoUtilities::DateTime dateTime);
bool LIB_SYNCTHING_CONNECTOR_EXPORT isLocal(const QUrl &url);

template <class Objects> QStringList LIB_SYNCTHING_CONNECTOR_EXPORT ids(const Objects &objects)
{
    QStringList ids;
    ids.reserve(objects.size());
    for (const auto &object : objects) {
        ids << object.id;
    }
    return ids;
}
}

#endif // DATA_UTILS_H
