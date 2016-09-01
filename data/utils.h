#ifndef DATA_UTILS_H
#define DATA_UTILS_H

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QString)

namespace ChronoUtilities {
class DateTime;
}

namespace Data {

QString agoString(ChronoUtilities::DateTime dateTime);

}

#endif // DATA_UTILS_H
