#ifndef SYNCTHINGWIDGETS_STATUSINFO_H
#define SYNCTHINGWIDGETS_STATUSINFO_H

#include "../global.h"

#include <QString>

QT_FORWARD_DECLARE_CLASS(QIcon)

namespace Data {
class SyncthingConnection;
}

namespace QtGui {

class SYNCTHINGWIDGETS_EXPORT StatusInfo {
public:
    explicit StatusInfo();
    explicit StatusInfo(const Data::SyncthingConnection &connection);

    QString statusText() const;
    const QIcon &statusIcon() const;
    void update(const Data::SyncthingConnection &connection);

private:
    QString m_statusText;
    const QIcon *m_statusIcon;
};

inline StatusInfo::StatusInfo(const Data::SyncthingConnection &connection)
{
    update(connection);
}

inline QString StatusInfo::statusText() const
{
    return m_statusText;
}

inline const QIcon &StatusInfo::statusIcon() const
{
    return *m_statusIcon;
}
}

#endif // SYNCTHINGWIDGETS_STATUSINFO_H
