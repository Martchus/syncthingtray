#include "./appservice.h"

#ifdef Q_OS_ANDROID
#include "./android.h"
#endif

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
#include <syncthing/interface.h>
#endif

#include <syncthingmodel/syncthingicons.h>

#include <QDebug>
#include <QStringBuilder>

#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QtCore/private/qandroidextras_p.h>
#include <QtEnvironmentVariables>
#endif

using namespace Data;

namespace QtGui {

/*!
 * \class AppService
 * \brief The AppService class manages the runtime of Syncthing for the Qt Quick GUI.
 * \remarks
 * - Under Android the app is split into two processes. One UI process and one service process. Hence the code
 *   that needs to run in the service process has been moved from the App class into the AppService class. The
 *   communication between these is implemented via Binder.
 * - Under other platforms an object of the AppService class is simply instantiated next to the main App object.
 *   There is no process boundary so communication between these is done via signals and slots.
 * - Under Android this class is accompanied by the Java class SyncthingService which implements certain
 *   Android-specific functionality in Java. Both classes also implement the notification handling because the
 *   Android service is a "foreground service" which means it is tied to a notification.
 * \sa https://developer.android.com/develop/background-work/services/fgs
 */

#ifdef SYNCTHINGTRAY_SERVICE_WITH_ICON_RENDERING
static constexpr auto textOnly = false;
#else
static constexpr auto textOnly = true;
#endif

/*!
 * \brief Initializes the Syncthing launcher and related platform-specific functionality.
 * \remarks
 * - Registers JNI functions for the Java class SyncthingService under Android. There is no
 *   "onNativeReady" call like in App because for services Qt itself provides this kind of
 *   synchronization.
 */
AppService::AppService(bool insecure, QObject *parent)
    : AppBase(insecure, textOnly, false, parent)
#ifdef Q_OS_ANDROID
    , m_clientsFollowingLog(false)
#endif
{
    qDebug() << "Initializing service app";

#ifdef Q_OS_ANDROID
    JniFn::registerServiceJniMethods(this);
#endif

#if defined(Q_OS_ANDROID) && defined(SYNCTHINGTRAY_SERVICE_WITH_ICON_RENDERING)
    // initialize experimental icon rendering within service to have icon on notification
    const auto scaleFactor = QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jfloat>("scaleFactor", "()F");
    const auto darkmode = QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jboolean>("isDarkmodeEnabled");
    qDebug() << "Scale factor for notification/service icons: " << scaleFactor;
    //qDebug() << "Darkmode for notification/service icons: " << darkmode;
    auto &iconManager = Data::IconManager::instance();
    //auto s = darkmode ? StatusIconSettings(StatusIconSettings::DarkTheme{}) : StatusIconSettings(StatusIconSettings::BrightTheme{});
    auto s = StatusIconSettings();
    s.renderSize *= 2 * scaleFactor;
    s.strokeWidth = StatusIconStrokeWidth::Thick;
    iconManager.applySettings(&s, &s, false, false);
    connect(&iconManager, &Data::IconManager::statusIconsChanged, this, &AppService::invalidateAndroidIconCache);
#endif

#ifdef Q_OS_ANDROID
    m_launcher.setManualStopHandler([] {
        QJniObject(QNativeInterface::QAndroidApplication::context())
            .callMethod<jint>("sendMessageToClients", static_cast<jint>(ActivityAction::FlagManualStop), 0, 0, QByteArray());
        return false;
    });
#endif

    connect(&m_connection, &SyncthingConnection::error, this, &AppService::handleConnectionError);
    connect(&m_connection, &SyncthingConnection::statusChanged, this, &AppService::handleConnectionStatusChanged);
    connect(&m_connection, &SyncthingConnection::newDevices, this, &AppService::handleChangedDevices);
    connect(&m_connection, &SyncthingConnection::autoReconnectIntervalChanged, this, &AppService::invalidateStatus);
    connect(&m_connection, &SyncthingConnection::hasOutOfSyncDirsChanged, this, &AppService::invalidateStatus);
    connect(&m_connection, &SyncthingConnection::devStatusChanged, this, &AppService::handleChangedDevices);
    connect(&m_connection, &SyncthingConnection::newErrors, this, &AppService::handleNewErrors);

    m_launcher.setEmittingOutput(true);
    connect(&m_launcher, &SyncthingLauncher::outputAvailable, this, &AppService::gatherLogs);
    connect(&m_launcher, &SyncthingLauncher::runningChanged, this, &AppService::handleRunningChanged);
    connect(&m_launcher, &SyncthingLauncher::runningChanged, this, &AppService::broadcastLauncherStatus);
    connect(&m_launcher, &SyncthingLauncher::guiUrlChanged, this, &AppService::handleGuiUrlChanged);
    connect(&m_launcher, &SyncthingLauncher::guiUrlChanged, this, &AppService::broadcastLauncherStatus);

#ifdef Q_OS_ANDROID
    connect(&m_notifier, &SyncthingNotifier::newDevice, this, &AppService::showNewDevice);
    connect(&m_notifier, &SyncthingNotifier::newDir, this, &AppService::showNewDir);
    connect(this, &AppService::error, this, &AppService::showError);
#endif

    m_connection.setPollingFlags(SyncthingConnection::PollingFlags::MainEvents | SyncthingConnection::PollingFlags::RemoteIndexUpdated
        | SyncthingConnection::PollingFlags::Errors);

    if (!SyncthingLauncher::mainInstance()) {
        SyncthingLauncher::setMainInstance(&m_launcher);
    }

    reloadSettings();
}

AppService::~AppService()
{
    qDebug() << "Destroying service";
    if (SyncthingLauncher::mainInstance() == &m_launcher) {
        SyncthingLauncher::setMainInstance(nullptr);
    }
#ifdef Q_OS_ANDROID
    JniFn::unregisterServiceJniMethods(this);
#endif
}

const QString &AppService::status()
{
    if (m_status.has_value()) {
        return *m_status;
    }
    if (m_connectToLaunched) {
        if (!m_launcher.isRunning()) {
            return m_status.emplace(m_launcher.runningStatus());
        } else if (m_launcher.isStarting()) {
            return m_status.emplace(tr("Backend is starting …"));
        }
    }
    return AppBase::status();
}

#ifdef Q_OS_ANDROID
/// \cond
static QByteArray serializeVariant(const QVariant &variant)
{
    auto res = QByteArray();
    auto stream = QDataStream(&res, QIODevice::WriteOnly);
    stream << variant;
    return res;
}
/// \endcond
#endif

void AppService::broadcastLauncherStatus()
{
    auto launcherStatus = m_launcher.overallStatus();
    launcherStatus[QStringLiteral("unixSocketPath")] = m_syncthingUnixSocketPath;
#ifdef Q_OS_ANDROID
    QJniObject(QNativeInterface::QAndroidApplication::context())
        .callMethod<jint>("sendMessageToClients", static_cast<jint>(ActivityAction::UpdateLauncherStatus), 0, 0, serializeVariant(launcherStatus));
#else
    emit launcherStatusChanged(launcherStatus);
#endif
}

bool AppService::applyLauncherSettings()
{
    const auto launcherSettingsObj = m_settings.value(QLatin1String("launcher")).toObject();
    const auto tweaksSettingsObj = m_settings.value(QLatin1String("tweaks")).toObject();
    m_launcher.setStoppingOnMeteredConnection(launcherSettingsObj.value(QLatin1String("stopOnMetered")).toBool());
    const auto shouldRun = launcherSettingsObj.value(QLatin1String("run")).toBool();
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    auto options = LibSyncthing::RuntimeOptions();
    auto customSyncthingHome = launcherSettingsObj.value(QLatin1String("stHomeDir")).toString();
    m_syncthingConfigDir = customSyncthingHome.isEmpty() ? m_settingsDir->path() + QStringLiteral("/syncthing") : customSyncthingHome;
    m_syncthingDataDir = m_syncthingConfigDir;
    options.configDir = m_syncthingConfigDir.toStdString();
    options.dataDir = options.configDir;
    if (tweaksSettingsObj.value(QLatin1String("useUnixDomainSocket")).toBool()) {
        m_syncthingUnixSocketPath = settingsDir().path() + QStringLiteral("/syncthing.socket");
        options.guiAddress = "unix://" + m_syncthingUnixSocketPath.toStdString();
        options.flags = options.flags | LibSyncthing::RuntimeFlags::SkipPortProbing;
    }
    m_launcher.setLibSyncthingLogLevel(launcherSettingsObj.value(QLatin1String("logLevel")).toString());
    if (launcherSettingsObj.value(QLatin1String("writeLogFile")).toBool()) {
        if (!m_launcher.logFile().isOpen()) {
            m_launcher.logFile().setFileName(syncthingLogFilePath());
            if (!m_launcher.logFile().open(QIODeviceBase::WriteOnly | QIODeviceBase::Append | QIODeviceBase::Text)) {
                emit error(tr("Unable to open persistent log file for Syncthing under \"%1\": %2")
                        .arg(m_launcher.logFile().fileName(), m_launcher.logFile().errorString()));
                return false;
            }
        }
    } else {
        m_launcher.logFile().close();
    }
#ifdef Q_OS_ANDROID
    if (shouldRun) {
        if (const auto ipObj = QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jstring>("getGatewayIPv4"); ipObj.isValid()) {
            const auto ip = ipObj.toString();
            qDebug() << "Setting FALLBACK_NET_GATEWAY_IPV4:" << ip;
            qputenv("FALLBACK_NET_GATEWAY_IPV4", ip.toUtf8());
        } else {
            qunsetenv("FALLBACK_NET_GATEWAY_IPV4");
        }
    }
#endif
    m_launcher.setRunning(shouldRun, std::move(options));
#else
    if (shouldRun) {
        emit error(tr("This build of the app cannot launch Syncthing."));
        return false;
    }
#endif
    return true;
}

bool AppService::reloadSettings()
{
    qDebug() << "Reloading settings";
    const auto res = loadSettings(true) && applyLauncherSettings();
    invalidateStatus();
    return res;
}

void AppService::terminateSyncthing()
{
    m_launcher.terminate();
}

void AppService::stopLibSyncthing()
{
    m_launcher.stopLibSyncthing();
}

void AppService::restartSyncthing()
{
    m_launcher.setManuallyStopped(true);
    m_connection.restart();
}

void AppService::shutdownSyncthing()
{
    m_launcher.setManuallyStopped(true);
    m_connection.shutdown();
}

void AppService::clearLog()
{
    m_log.clear();
}

void AppService::replayLog()
{
#ifdef Q_OS_ANDROID
    m_clientsFollowingLog = true;
    QJniObject(QNativeInterface::QAndroidApplication::context())
        .callMethod<jint>("sendMessageToClients", static_cast<jint>(ActivityAction::AppendLog), 0, 0, m_log);
#else
    emit logsAvailable(m_log);
#endif
}

#ifdef Q_OS_ANDROID
void AppService::showError(const QString &error)
{
    const auto ctx = QJniObject(QNativeInterface::QAndroidApplication::context());
    if (ctx.callMethod<jint>("sendMessageToClients", static_cast<jint>(ActivityAction::ShowError), 0, 0, error) > 0) {
        return;
    }
    const auto title = QJniObject::fromString(tr("Syncthing App ran into error"));
    const auto text = QJniObject::fromString(error);
    const auto subText = QJniObject::fromString(QString());
    static const auto page = QJniObject::fromString(QStringLiteral("internalErrors"));
#ifdef SYNCTHINGTRAY_SERVICE_WITH_ICON_RENDERING
    const auto &icon = makeAndroidIcon(commonForkAwesomeIcons().exclamation);
#else
    static const auto icon = QJniObject();
#endif
    updateExtraAndroidNotification(title, text, subText, page, icon);
}

void AppService::clearInternalErrors()
{
    clearAndroidExtraNotifications(3, 3 + m_internalErrors.size());
}

void AppService::handleMessageFromActivity(ServiceAction action, int arg1, int arg2, const QString &str)
{
    Q_UNUSED(arg1)
    Q_UNUSED(arg2)
    Q_UNUSED(str)
    qDebug() << "Received message from activity: " << static_cast<int>(action);
    switch (action) {
    case ServiceAction::ReloadSettings:
        QMetaObject::invokeMethod(this, "reloadSettings", Qt::QueuedConnection);
        break;
    case ServiceAction::TerminateSyncthing:
        QMetaObject::invokeMethod(this, "terminateSyncthing", Qt::QueuedConnection);
        break;
    case ServiceAction::RestartSyncthing:
        QMetaObject::invokeMethod(this, "restartSyncthing", Qt::QueuedConnection);
        break;
    case ServiceAction::ShutdownSyncthing:
        QMetaObject::invokeMethod(this, "shotdownSyncthing", Qt::QueuedConnection);
        break;
    case ServiceAction::ConnectToSyncthing:
        QMetaObject::invokeMethod(connection(), "connect", Qt::QueuedConnection);
        break;
    case ServiceAction::BroadcastLauncherStatus:
        QMetaObject::invokeMethod(this, "broadcastLauncherStatus", Qt::QueuedConnection);
        break;
    case ServiceAction::Reconnect:
        QMetaObject::invokeMethod(connection(), "reconnect", Qt::QueuedConnection);
        break;
    case ServiceAction::ClearInternalErrorNotifications:
        QMetaObject::invokeMethod(this, "clearInternalErrors", Qt::QueuedConnection);
        break;
    case ServiceAction::ClearLog:
        QMetaObject::invokeMethod(this, "clearLog", Qt::QueuedConnection);
        break;
    case ServiceAction::FollowLog:
        QMetaObject::invokeMethod(this, "replayLog", Qt::QueuedConnection);
        break;
    case ServiceAction::CloseLog:
        m_clientsFollowingLog = false;
        break;
    case ServiceAction::RequestErrors:
        QMetaObject::invokeMethod(connection(), "requestErrors", Qt::QueuedConnection);
        break;
    default:;
    }
}
#endif

void AppService::handleConnectionError(
    const QString &errorMessage, Data::SyncthingErrorCategory category, int networkError, const QNetworkRequest &request, const QByteArray &response)
{
    if (!InternalError::isRelevant(m_connection, category, errorMessage, networkError, false)) {
        return;
    }
#ifdef Q_OS_ANDROID
    auto error = InternalError(errorMessage, request.url(), response);
    showInternalError(error);
#else
    Q_UNUSED(errorMessage)
    Q_UNUSED(request)
    Q_UNUSED(response)
#endif
}

void AppService::invalidateStatus()
{
    AppBase::invalidateStatus();
#ifdef Q_OS_ANDROID
    updateAndroidNotification();
#endif
}

void AppService::gatherLogs(const QByteArray &newOutput)
{
    const auto asText = QString::fromUtf8(newOutput);
#ifdef Q_OS_ANDROID
    if (m_clientsFollowingLog) {
        QJniObject(QNativeInterface::QAndroidApplication::context())
            .callMethod<jint>("sendMessageToClients", static_cast<jint>(ActivityAction::AppendLog), 0, 0, asText);
    }
#else
    emit logsAvailable(asText);
#endif
    m_log.append(asText);
}

void AppService::handleRunningChanged(bool isRunning)
{
    Q_UNUSED(isRunning)
    if (m_connectToLaunched) {
        invalidateStatus();
    }
}

void AppService::handleChangedDevices()
{
    m_statusInfo.updateConnectedDevices(m_connection);
#ifdef Q_OS_ANDROID
    updateAndroidNotification();
#endif
}

void AppService::handleNewErrors(const std::vector<Data::SyncthingError> &errors)
{
    Q_UNUSED(errors)
    invalidateStatus();
#ifdef Q_OS_ANDROID
    updateSyncthingErrorsNotification(errors);
#endif
}

void AppService::handleConnectionStatusChanged(Data::SyncthingStatus newStatus)
{
    invalidateStatus();
#ifdef Q_OS_ANDROID
    switch (newStatus) {
    case Data::SyncthingStatus::Reconnecting:
        clearSyncthingErrorsNotification();
        break;
    default:;
    }
#else
    Q_UNUSED(newStatus)
#endif
}

#ifdef Q_OS_ANDROID
#ifdef SYNCTHINGTRAY_SERVICE_WITH_ICON_RENDERING
void AppService::invalidateAndroidIconCache()
{
    m_statusInfo.updateConnectionStatus(m_connection);
    m_androidIconCache.clear();
    updateAndroidNotification();
}

QJniObject &AppService::makeAndroidIcon(const QIcon &icon)
{
    auto &cachedIcon = m_androidIconCache[&icon];
    if (!cachedIcon.isValid()) {
        cachedIcon = IconManager::makeAndroidBitmap(icon.pixmap(QSize(32, 32)).toImage());
    }
    return cachedIcon;
}
#endif

void AppService::updateAndroidNotification()
{
    const auto title = QJniObject::fromString(m_connection.isConnected() ? m_statusInfo.statusText() : status());
    const auto text = QJniObject::fromString(m_statusInfo.additionalStatusText());
    static const auto subText = QJniObject::fromString(QString());
#ifdef SYNCTHINGTRAY_SERVICE_WITH_ICON_RENDERING
    const auto &icon = makeAndroidIcon(m_statusInfo.statusIcon());
#else
    static const auto icon = QJniObject();
#endif
    QJniObject::callStaticMethod<void>("io/github/martchus/syncthingtray/SyncthingService", "updateNotification",
        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Landroid/graphics/Bitmap;)V", title.object(), text.object(), subText.object(),
        icon.object());
}

void AppService::updateExtraAndroidNotification(
    const QJniObject &title, const QJniObject &text, const QJniObject &subText, const QJniObject &page, const QJniObject &icon, int id)
{
    QJniObject::callStaticMethod<void>("io/github/martchus/syncthingtray/SyncthingService", "updateExtraNotification",
        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Landroid/graphics/Bitmap;I)V", title.object(), text.object(),
        subText.object(), page.object(), icon.object(), id ? id : ++m_androidNotificationId);
}

void AppService::clearAndroidExtraNotifications(int firstId, int lastId)
{
    QJniObject::callStaticMethod<void>("io/github/martchus/syncthingtray/SyncthingService", "cancelExtraNotification", "(II)V", firstId, lastId);
}

void AppService::updateSyncthingErrorsNotification(const std::vector<Data::SyncthingError> &newErrors)
{
    if (newErrors.empty()) {
        clearSyncthingErrorsNotification();
        return;
    }
    const auto syncthingErrors = newErrors.size();
    const auto &mostRecent = newErrors.back();
    const auto title = QJniObject::fromString(
        syncthingErrors == 1 ? tr("Syncthing error/notification") : tr("%1 Syncthing errors/notifications").arg(syncthingErrors));
    const auto text = QJniObject::fromString(syncthingErrors == 1 ? mostRecent.message : tr("Most recent: ") + mostRecent.message);
    const auto subText = QJniObject::fromString(QString::fromStdString(mostRecent.when.toString()));
    static const auto page = QJniObject::fromString(QStringLiteral("connectionErrors"));
#ifdef SYNCTHINGTRAY_SERVICE_WITH_ICON_RENDERING
    const auto &icon = makeAndroidIcon(commonForkAwesomeIcons().exclamation);
#else
    static const auto icon = QJniObject();
#endif
    updateExtraAndroidNotification(title, text, subText, page, icon, 2);
}

void AppService::clearSyncthingErrorsNotification()
{
    clearAndroidExtraNotifications(2, -1);
}

void AppService::showInternalError(const InternalError &error)
{
    const auto title = QJniObject::fromString(
        m_internalErrors.empty() ? tr("Syncthing API error") : tr("%1 Syncthing API errors").arg(m_internalErrors.size() + 1));
    const auto text = QJniObject::fromString(m_internalErrors.empty() ? error.message : tr("Most recent: ") + error.message);
    const auto subText = QJniObject::fromString(error.url.isEmpty() ? QString() : QStringLiteral("URL: ") + error.url.toString());
    static const auto page = QJniObject::fromString(QStringLiteral("internalErrors"));
#ifdef SYNCTHINGTRAY_SERVICE_WITH_ICON_RENDERING
    const auto &icon = makeAndroidIcon(commonForkAwesomeIcons().exclamation);
#else
    static const auto icon = QJniObject();
#endif
    updateExtraAndroidNotification(title, text, subText, page, icon, 3);
}

void AppService::showNewDevice(const QString &devId, const QString &message)
{
    const auto title = QJniObject::fromString(tr("Syncthing device wants to connect"));
    const auto text = QJniObject::fromString(message);
    static const auto subText = QJniObject::fromString(QString());
    const auto page = QJniObject::fromString(QStringLiteral("newdev:") + devId);
#ifdef SYNCTHINGTRAY_SERVICE_WITH_ICON_RENDERING
    const auto &icon = makeAndroidIcon(commonForkAwesomeIcons().networkWired);
#else
    static const auto icon = QJniObject();
#endif
    updateExtraAndroidNotification(title, text, subText, page, icon);
}

void AppService::showNewDir(const QString &devId, const QString &dirId, const QString &dirLabel, const QString &message)
{
    const auto title = QJniObject::fromString(tr("Syncthing device wants to share folder"));
    const auto text = QJniObject::fromString(message);
    static const auto subText = QJniObject::fromString(QString());
    const auto page = QJniObject::fromString(QStringLiteral("newfolder:") % devId % QChar(':') % dirId % QChar(':') % dirLabel);
#ifdef SYNCTHINGTRAY_SERVICE_WITH_ICON_RENDERING
    const auto &icon = makeAndroidIcon(commonForkAwesomeIcons().shareAlt);
#else
    static const auto icon = QJniObject();
#endif
    updateExtraAndroidNotification(title, text, subText, page, icon);
}
#endif

} // namespace QtGui
