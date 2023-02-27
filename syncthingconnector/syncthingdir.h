#ifndef DATA_SYNCTHINGDIR_H
#define DATA_SYNCTHINGDIR_H

#include "./qstringhash.h"
#include "./syncthingcompletion.h"

#include <qtutilities/misc/compat.h>

#include <c++utilities/chrono/datetime.h>

#include <QFileInfo>
#include <QJsonObject>
#include <QMetaType>
#include <QString>

#include <unordered_map>
#include <vector>

namespace Data {

/// \brief The SyncthingDirStatus enum represents a Syncthing directory status.
/// \remarks
/// - It needs to be kept in sync with the states defined in Syncthing's "syncthing/lib/model/folderstate.go". If it is not in sync
///   SyncthingDirStatus::Unknown will be used.
/// - There's no real documentation here because these enum items really correspond to the folder state as provided by Syncthing.
/// - When changing this enum, also change SyncthingDir::assignStatus(), SyncthingConnection::setStatus(), SyncthingDirActions::updateStatus()
///   and SyncthingDirectoryModel::data(), SyncthingDirectoryModel::dirStatusString() and SyncthingDirectoryModel::dirStatusColor().
enum class SyncthingDirStatus {
    Unknown, /**< directory status is unknown */
    Idle, /**< directory is idling ("idle") */
    Scanning, /**< directory is scanning ("scanning") */
    WaitingToScan, /**< directory is waiting to scan ("scan-waiting") */
    WaitingToSync, /**< directory is waiting to sync ("sync-waiting") */
    PreparingToSync, /**< directory is preparing to sync ("sync-preparing") */
    Synchronizing, /**< directory is synchronizing ("syncing") */
    Cleaning, /**< directory is cleaning ("cleaning") */
    WaitingToClean, /**< directory is waiting to clean ("clean-waiting") */
    OutOfSync, /**< directory is out-of-sync due to errors ("error") */
};

LIB_SYNCTHING_CONNECTOR_EXPORT QString statusString(SyncthingDirStatus status);

enum class SyncthingDirType { Unknown, SendReceive, SendOnly, ReceiveOnly };

LIB_SYNCTHING_CONNECTOR_EXPORT QString dirTypeString(SyncthingDirType dirType);

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingItemError {
    explicit SyncthingItemError(const QString &message = QString(), const QString &path = QString())
        : message(message)
        , path(path)
    {
    }

    bool operator==(const SyncthingItemError &other) const
    {
        return message == other.message && path == other.path;
    }

    QString message;
    QString path;
};

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingFileChange {
    QString action;
    QString type;
    QString modifiedBy;
    QString path;
    CppUtilities::DateTime eventTime;
    bool local = false;
};

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingItemDownloadProgress {
    explicit SyncthingItemDownloadProgress(
        const QString &containingDirPath = QString(), const QString &relativeItemPath = QString(), const QJsonObject &values = QJsonObject());
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
    CppUtilities::DateTime lastUpdate;
    static constexpr unsigned int syncthingBlockSize = 128 * 1024;
};

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingStatistics {
    Q_GADGET
    Q_PROPERTY(quint64 bytes MEMBER bytes)
    Q_PROPERTY(quint64 deletes MEMBER deletes)
    Q_PROPERTY(quint64 dirs MEMBER dirs)
    Q_PROPERTY(quint64 files MEMBER files)
    Q_PROPERTY(quint64 symlinks MEMBER symlinks)

public:
    quint64 bytes = 0;
    quint64 deletes = 0;
    quint64 dirs = 0;
    quint64 files = 0;
    quint64 symlinks = 0;

    constexpr bool isNull() const;
    constexpr bool operator==(const SyncthingStatistics &other) const;
    constexpr bool operator!=(const SyncthingStatistics &other) const;
    SyncthingStatistics &operator+=(const SyncthingStatistics &other);
};

constexpr bool SyncthingStatistics::isNull() const
{
    return bytes == 0 && deletes == 0 && dirs == 0 && files == 0 && symlinks == 0;
}

constexpr bool SyncthingStatistics::operator==(const SyncthingStatistics &other) const
{
    return bytes == other.bytes && deletes == other.deletes && dirs == other.dirs && files == other.files && symlinks == other.symlinks;
}

constexpr bool SyncthingStatistics::operator!=(const SyncthingStatistics &other) const
{
    return !(*this == other);
}

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingDir {
    explicit SyncthingDir(const QString &id = QString(), const QString &label = QString(), const QString &path = QString());
    bool assignStatus(const QString &statusStr, CppUtilities::DateTime time);
    bool assignStatus(SyncthingDirStatus newStatus, CppUtilities::DateTime time);
    bool assignDirType(const QString &dirType);
    const QString &displayName() const;
    QString statusString() const;
    QString dirTypeString() const;
    QtUtilities::StringView pathWithoutTrailingSlash() const;
    bool isLocallyUpToDate() const;
    bool areRemotesUpToDate() const;
    bool isUnshared() const;

    QString id;
    QString label;
    QString path;
    QStringList deviceIds;
    QStringList deviceNames;
    SyncthingDirType dirType = SyncthingDirType::Unknown;
    int rescanInterval = 0;
    int minDiskFreePercentage = 0;
    SyncthingDirStatus status = SyncthingDirStatus::Unknown;
    CppUtilities::DateTime lastStatusUpdate;
    CppUtilities::DateTime lastSyncStarted;
    int completionPercentage = 0;
    int scanningPercentage = 0;
    double scanningRate = 0;
    double fileSystemWatcherDelay = 0.0;
    std::unordered_map<QString, SyncthingCompletion> completionByDevice;
    QString globalError;
    quint64 pullErrorCount = 0;
    std::vector<SyncthingItemError> itemErrors;
    std::vector<SyncthingFileChange> recentChanges;
    SyncthingStatistics globalStats, localStats, neededStats;
    CppUtilities::DateTime lastStatisticsUpdate;
    CppUtilities::DateTime lastScanTime;
    CppUtilities::DateTime lastFileTime;
    QString lastFileName;
    std::vector<SyncthingItemDownloadProgress> downloadingItems;
    int blocksAlreadyDownloaded = 0;
    int blocksToBeDownloaded = 0;
    QString downloadLabel;
    QString rawStatus;
    unsigned int downloadPercentage = 0;
    bool ignorePermissions = false;
    bool ignoreDelete = false;
    bool ignorePatterns = false;
    bool autoNormalize = false;
    bool lastFileDeleted = false;
    bool fileSystemWatcherEnabled = false;
    bool paused = false;

private:
    bool checkWhetherStatusUpdateRelevant(CppUtilities::DateTime time);
    bool finalizeStatusUpdate(SyncthingDirStatus newStatus, CppUtilities::DateTime time);
};

inline SyncthingDir::SyncthingDir(const QString &id, const QString &label, const QString &path)
    : id(id)
    , label(label)
    , path(path)
{
}

inline const QString &SyncthingDir::displayName() const
{
    return label.isEmpty() ? id : label;
}

inline QString SyncthingDir::dirTypeString() const
{
    return ::Data::dirTypeString(dirType);
}

inline bool SyncthingDir::isLocallyUpToDate() const
{
    return neededStats.isNull();
}

inline bool SyncthingDir::isUnshared() const
{
    return deviceIds.empty() && (status == SyncthingDirStatus::Idle || status == SyncthingDirStatus::Unknown);
}

inline bool SyncthingDir::assignStatus(SyncthingDirStatus newStatus, CppUtilities::DateTime time)
{
    if (!checkWhetherStatusUpdateRelevant(time)) {
        return false;
    }
    rawStatus.clear();
    return finalizeStatusUpdate(newStatus, time);
}

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingOverallDirStatistics {
    Q_GADGET
    Q_PROPERTY(SyncthingStatistics local MEMBER local)
    Q_PROPERTY(SyncthingStatistics global MEMBER global)
    Q_PROPERTY(SyncthingStatistics needed MEMBER needed)

public:
    explicit SyncthingOverallDirStatistics();
    explicit SyncthingOverallDirStatistics(const std::vector<SyncthingDir> &directories);

    SyncthingStatistics local;
    SyncthingStatistics global;
    SyncthingStatistics needed;

    bool isNull() const;
};

inline SyncthingOverallDirStatistics::SyncthingOverallDirStatistics()
{
}

inline bool SyncthingOverallDirStatistics::isNull() const
{
    return local.isNull() && global.isNull();
}

} // namespace Data

Q_DECLARE_METATYPE(Data::SyncthingItemError)
Q_DECLARE_METATYPE(Data::SyncthingItemDownloadProgress)
Q_DECLARE_METATYPE(Data::SyncthingStatistics)
Q_DECLARE_METATYPE(Data::SyncthingOverallDirStatistics)
Q_DECLARE_METATYPE(Data::SyncthingDir)

#endif // DATA_SYNCTHINGDIR_H
