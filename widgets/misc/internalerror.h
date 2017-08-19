#ifndef SYNCTHINGWIDGETS_INTERNALERROR_H
#define SYNCTHINGWIDGETS_INTERNALERROR_H

#include "../global.h"

#include <c++utilities/chrono/datetime.h>

#include <QByteArray>
#include <QString>
#include <QUrl>

namespace QtGui {

struct SYNCTHINGWIDGETS_EXPORT InternalError {
    InternalError(const QString &message = QString(), const QUrl &url = QUrl(), const QByteArray &response = QByteArray());

    QString message;
    QUrl url;
    QByteArray response;
    ChronoUtilities::DateTime when;
};

inline InternalError::InternalError(const QString &message, const QUrl &url, const QByteArray &response)
    : message(message)
    , url(url)
    , response(response)
    , when(ChronoUtilities::DateTime::now())
{
}
}

#endif // SYNCTHINGWIDGETS_INTERNALERROR_H
