#ifndef SYNCTHINGWIDGETS_INTERNAL_ERROR_H
#define SYNCTHINGWIDGETS_INTERNAL_ERROR_H

#include "../global.h"

#include <c++utilities/chrono/datetime.h>

#include <QByteArray>
#include <QString>
#include <QUrl>

namespace Data {
class SyncthingConnection;
enum class SyncthingErrorCategory;
} // namespace Data

namespace QtGui {

struct SYNCTHINGWIDGETS_EXPORT InternalError {
    explicit InternalError(const QString &message = QString(), const QUrl &url = QUrl(), const QByteArray &response = QByteArray());

    static bool isRelevant(const Data::SyncthingConnection &connection, Data::SyncthingErrorCategory category, int networkError);

    QString message;
    QUrl url;
    QByteArray response;
    CppUtilities::DateTime when;
};

/*!
 * \brief Constructs a new error suitable for display purposes (password in \a url is redacted).
 */
inline InternalError::InternalError(const QString &message, const QUrl &url, const QByteArray &response)
    : message(message)
    , url(url)
    , response(response)
    , when(CppUtilities::DateTime::now())
{
    if (!this->url.password().isEmpty()) {
        this->url.setPassword(QStringLiteral("redacted"));
    }
}

} // namespace QtGui

#endif // SYNCTHINGWIDGETS_INTERNAL_ERROR_H
