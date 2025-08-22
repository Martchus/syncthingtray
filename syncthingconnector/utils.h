#ifndef DATA_UTILS_H
#define DATA_UTILS_H

#include "./global.h"

#include <c++utilities/misc/traits.h>

#include <QJsonValue>
#include <QStringList>
#include <QUrl>

#include <limits>
#include <utility>
#include <vector>

QT_FORWARD_DECLARE_CLASS(QJsonObject)
QT_FORWARD_DECLARE_CLASS(QHostAddress)
QT_FORWARD_DECLARE_CLASS(QNetworkInformation)

#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
#define SYNCTHINGCONNECTION_SUPPORT_METERED
#endif

namespace CppUtilities {
class DateTime;
}

namespace Data {

namespace Traits = CppUtilities::Traits;

struct SyncthingStatistics;
struct SyncthingDir;
struct SyncthingDev;

LIB_SYNCTHING_CONNECTOR_EXPORT QString agoString(CppUtilities::DateTime dateTime);
LIB_SYNCTHING_CONNECTOR_EXPORT QString trafficString(std::uint64_t total, double rate);
LIB_SYNCTHING_CONNECTOR_EXPORT QString directoryStatusString(const Data::SyncthingStatistics &stats);
LIB_SYNCTHING_CONNECTOR_EXPORT QString syncCompleteString(
    const std::vector<const SyncthingDir *> &completedDirs, const SyncthingDev *remoteDevice = nullptr);
LIB_SYNCTHING_CONNECTOR_EXPORT QString rescanIntervalString(int rescanInterval, bool fileSystemWatcherEnabled);
LIB_SYNCTHING_CONNECTOR_EXPORT QString stripPort(const QString &address);
LIB_SYNCTHING_CONNECTOR_EXPORT bool isLocal(const QString &hostName);
LIB_SYNCTHING_CONNECTOR_EXPORT bool isLocal(const QString &hostName, const QHostAddress &hostAddress);
LIB_SYNCTHING_CONNECTOR_EXPORT bool setDirectoriesPaused(QJsonObject &syncthingConfig, const QStringList &dirIds, bool paused);
LIB_SYNCTHING_CONNECTOR_EXPORT bool setDevicesPaused(QJsonObject &syncthingConfig, const QStringList &dirs, bool paused);
LIB_SYNCTHING_CONNECTOR_EXPORT QString substituteTilde(const QString &path, const QString &tilde, const QString &pathSeparator);
#ifdef SYNCTHINGCONNECTION_SUPPORT_METERED
LIB_SYNCTHING_CONNECTOR_EXPORT std::pair<const QNetworkInformation *, bool> loadNetworkInformationBackendForMetered(
    bool determineInitialValue = false);
#endif

/*!
 * \brief Returns whether the host specified by the given \a url is the local machine.
 */
inline bool isLocal(const QUrl &url)
{
    if (isLocal(url.host())) {
        return true;
    }
    const auto scheme = url.scheme();
    return scheme.startsWith(QLatin1String("unix+")) || scheme.startsWith(QLatin1String("local+"));
}

template <typename IntType = quint64, Traits::EnableIf<std::is_integral<IntType>> * = nullptr>
inline LIB_SYNCTHING_CONNECTOR_EXPORT IntType jsonValueToInt(const QJsonValue &value, double defaultValue = 0.0)
{
    return static_cast<IntType>(value.toDouble(defaultValue));
}

constexpr LIB_SYNCTHING_CONNECTOR_EXPORT int trQuandity(quint64 quandity)
{
    return quandity > std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : static_cast<int>(quandity);
}

template <class Objects, class Accessor> LIB_SYNCTHING_CONNECTOR_EXPORT QStringList things(const Objects &objects, Accessor accessor)
{
    QStringList things;
    things.reserve(static_cast<QStringList::size_type>(objects.size()));
    for (const auto &object : objects) {
        things << accessor(object);
    }
    return things;
}

template <class Objects> LIB_SYNCTHING_CONNECTOR_EXPORT QStringList ids(const Objects &objects)
{
    return things(objects, [](const auto &object) { return Traits::dereferenceMaybe(object).id; });
}

template <class Objects> LIB_SYNCTHING_CONNECTOR_EXPORT QStringList displayNames(const Objects &objects)
{
    return things(objects, [](const auto &object) { return Traits::dereferenceMaybe(object).displayName(); });
}

} // namespace Data

#endif // DATA_UTILS_H
