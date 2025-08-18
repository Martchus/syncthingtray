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
    explicit StatusInfo(bool textOnly = false, bool clickToConnect = false);
    explicit StatusInfo(
        const Data::SyncthingConnection &connection, const QString &configurationName = QString(), bool textOnly = false, bool clickToConnect = true);

    const QString &statusText() const;
    const QString &additionalStatusText() const;
    const QIcon &statusIcon() const;
    void updateConnectionStatus(const Data::SyncthingConnection &connection, const QString &configurationName = QString());
    void updateConnectedDevices(const Data::SyncthingConnection &connection);

private:
    void recomputeAdditionalStatusText();

    QString m_statusText;
    QString m_additionalStatusInfo;
    QString m_additionalDeviceInfo;
    QString m_additionalStatusText;
    const QIcon *m_statusIcon;
    bool m_textOnly;
    bool m_clickToConnect;
};

inline StatusInfo::StatusInfo(const Data::SyncthingConnection &connection, const QString &configurationName, bool textOnly, bool clickToConnect)
    : m_statusIcon(nullptr)
    , m_textOnly(textOnly)
    , m_clickToConnect(clickToConnect)
{
    updateConnectionStatus(connection, configurationName);
    updateConnectedDevices(connection);
}

inline const QString &StatusInfo::statusText() const
{
    return m_statusText;
}

inline const QString &StatusInfo::additionalStatusText() const
{
    return m_additionalStatusText;
}

inline const QIcon &StatusInfo::statusIcon() const
{
    return *m_statusIcon;
}
} // namespace QtGui

#endif // SYNCTHINGWIDGETS_STATUSINFO_H
