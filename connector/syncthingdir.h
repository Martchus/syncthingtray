#ifndef DATA_SYNCTHINGDIR_H
#define DATA_SYNCTHINGDIR_H

#include "./global.h"

#include <c++utilities/chrono/datetime.h>

#include <QString>
#include <QMetaType>
#include <QFileInfo>
#include <QJsonObject>

namespace Data {
Q_NAMESPACE

enum class SyncthingDirStatus
{
    Unknown,
    Idle,
    Unshared,
    Scanning,
    Synchronizing,
    OutOfSync
};
Q_ENUM_NS(SyncthingDirStatus)

QString statusString(SyncthingDirStatus status);

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingDirError
{
    SyncthingDirError(const QString &message = QString(), const QString &path = QString()) :
        message(message),
        path(path)
    {}

    bool operator ==(const SyncthingDirError &other) const
    {
        return message == other.message && path == other.path;
    }

    QString message;
    QString path;
};

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingItemDownloadProgress
{
    SyncthingItemDownloadProgress(const QString &containingDirPath = QString(), const QString &relativeItemPath = QString(), const QJsonObject &values = QJsonObject());
    QString relativePath;
    QFileInfo fileInfo;
    int blocksCurrentlyDownloading = 0;
    int blocksAlreadyDownloaded = 0;
    int totalNumberOfBlocks = 0;
    unsigned int downloadPercentage = 0;
    int blocksCopiedFromOrigin = 0;
    int blocksCopiedFromElsewhere = 0;
    int blocksReused = 0;
    int bytesAlreadyHandled;
    int totalNumberOfBytes = 0;
    QString label;
    ChronoUtilities::DateTime lastUpdate;
    static constexpr unsigned int syncthingBlockSize = 128 * 1024;
};

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingDir
{
    SyncthingDir(const QString &id = QString(), const QString &label = QString(), const QString &path = QString());
    bool assignStatus(const QString &statusStr, ChronoUtilities::DateTime time);
    bool assignStatus(SyncthingDirStatus newStatus, ChronoUtilities::DateTime time);
    QString displayName() const;
    QString statusString() const;
    QStringRef pathWithoutTrailingSlash() const;

    QString id;
    QString label;
    QString path;
    QStringList devices;
    bool readOnly = false;
    bool ignorePermissions = false;
    bool autoNormalize = false;
    int rescanInterval = 0;
    int minDiskFreePercentage = 0;
    SyncthingDirStatus status = SyncthingDirStatus::Idle;
    bool paused = false;
    ChronoUtilities::DateTime lastStatusUpdate;
    int progressPercentage = 0;
    int progressRate = 0;
    std::vector<SyncthingDirError> errors;
    std::vector<SyncthingDirError> previousErrors;
    int globalBytes = 0, globalDeleted = 0, globalFiles = 0;
    int localBytes = 0, localDeleted = 0, localFiles = 0;
    int neededByted = 0, neededFiles = 0;
    ChronoUtilities::DateTime lastScanTime;
    ChronoUtilities::DateTime lastFileTime;
    QString lastFileName;
    bool lastFileDeleted = false;
    std::vector<SyncthingItemDownloadProgress> downloadingItems;
    int blocksAlreadyDownloaded = 0;
    int blocksToBeDownloaded = 0;
    unsigned int downloadPercentage = 0;
    QString downloadLabel;
};

inline SyncthingDir::SyncthingDir(const QString &id, const QString &label, const QString &path) :
    id(id),
    label(label),
    path(path)
{}

inline QString SyncthingDir::displayName() const
{
    return label.isEmpty() ? id : label;
}

} // namespace Data

Q_DECLARE_METATYPE(Data::SyncthingDirError)
Q_DECLARE_METATYPE(Data::SyncthingItemDownloadProgress)
Q_DECLARE_METATYPE(Data::SyncthingDir)

#endif // DATA_SYNCTHINGDIR_H
