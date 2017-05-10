#ifndef DATA_SYNCTHINGDEV_H
#define DATA_SYNCTHINGDEV_H

#include "./global.h"

#include <c++utilities/chrono/datetime.h>

#include <QMetaType>
#include <QString>
#include <QStringList>

namespace Data {
#undef Q_NAMESPACE
#define Q_NAMESPACE
Q_NAMESPACE
extern LIB_SYNCTHING_CONNECTOR_EXPORT const QMetaObject staticMetaObject;
QT_ANNOTATE_CLASS(qt_qnamespace, "") /*end*/

enum class SyncthingDevStatus { Unknown, Disconnected, OwnDevice, Idle, Synchronizing, OutOfSync, Rejected };
Q_ENUM_NS(SyncthingDevStatus)

QString statusString(SyncthingDevStatus status);

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingDev {
    SyncthingDev(const QString &id = QString(), const QString &name = QString());
    QString statusString() const;

    QString id;
    QString name;
    QStringList addresses;
    QString compression;
    QString certName;
    SyncthingDevStatus status;
    int progressPercentage = 0;
    int progressRate = 0;
    bool introducer = false;
    bool paused = false;
    uint64 totalIncomingTraffic = 0;
    uint64 totalOutgoingTraffic = 0;
    QString connectionAddress;
    QString connectionType;
    QString clientVersion;
    ChronoUtilities::DateTime lastSeen;
};

inline SyncthingDev::SyncthingDev(const QString &id, const QString &name)
    : id(id)
    , name(name)
{
}

} // namespace Data

Q_DECLARE_METATYPE(Data::SyncthingDev)

#endif // DATA_SYNCTHINGDEV_H
