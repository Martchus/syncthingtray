#ifndef SYNCTHINGWIDGETS_INTERNAL_ERROR_H
#define SYNCTHINGWIDGETS_INTERNAL_ERROR_H

#include "../global.h"

#include <c++utilities/chrono/datetime.h>

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QUrl>

namespace Data {
class SyncthingConnection;
enum class SyncthingErrorCategory;
} // namespace Data

namespace QtGui {

struct SYNCTHINGWIDGETS_EXPORT InternalError {
    Q_GADGET
    Q_PROPERTY(QString message MEMBER message CONSTANT)
    Q_PROPERTY(QUrl url MEMBER url CONSTANT)
    Q_PROPERTY(QByteArray response MEMBER response CONSTANT)
    Q_PROPERTY(QString when READ whenToString CONSTANT)

public:
    explicit InternalError(const QString &message = QString(), const QUrl &url = QUrl(), const QByteArray &response = QByteArray());

    static bool isRelevant(const Data::SyncthingConnection &connection, Data::SyncthingErrorCategory category, const QString &message,
        int networkError, bool useGlobalSettings = true, bool isManuallyStopped = false);
    QString whenToString() const;

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

inline QString InternalError::whenToString() const
{
    return QString::fromStdString(when.toString());
}

// define (de)serialization operations used by the Qt Quick GUI on Android (only works as of Qt 6)
#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
inline QDataStream &operator<<(QDataStream &stream, const InternalError &error)
{
    return stream << error.message << error.url << error.response << static_cast<quint64>(error.when.totalTicks());
}

inline QDataStream &operator>>(QDataStream &stream, InternalError &error)
{
    auto ticks = quint64();
    stream >> error.message >> error.url >> error.response >> ticks;
    error.when = CppUtilities::DateTime(ticks);
    return stream;
}
#endif

} // namespace QtGui

#endif // SYNCTHINGWIDGETS_INTERNAL_ERROR_H
