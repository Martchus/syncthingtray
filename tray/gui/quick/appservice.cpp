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

using namespace Data;

namespace QtGui {

AppService::AppService(bool insecure, QObject *parent)
    : AppBase(insecure, parent)
{
    qDebug() << "Initializing service app";

#ifdef Q_OS_ANDROID
    JniFn::registerServiceJniMethods(this);
#endif

#ifdef Q_OS_ANDROID
    connect(&initIconManager(), &Data::IconManager::statusIconsChanged, this, &AppService::invalidateAndroidIconCache);
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

    m_connection.setPollingFlags(SyncthingConnection::PollingFlags::MainEvents | SyncthingConnection::PollingFlags::RemoteIndexUpdated | SyncthingConnection::PollingFlags::Errors);

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

void AppService::broadcastLauncherStatus()
{
#ifdef Q_OS_ANDROID
    auto intent = QAndroidIntent(QStringLiteral("io.github.martchus.syncthingtray.launcherstatus"));
    intent.putExtra(QStringLiteral("status"), m_launcher.overallStatus());
    QJniObject(QNativeInterface::QAndroidApplication::context())
        .callMethod<void>("sendBroadcast", "(Landroid/content/Intent;)V", intent.handle().object());
#else
    emit launcherStatusChanged(m_launcher.overallStatus());
#endif
}

bool AppService::applyLauncherSettings()
{
    const auto launcherSettingsObj = m_settings.value(QLatin1String("launcher")).toObject();
    m_launcher.setStoppingOnMeteredConnection(launcherSettingsObj.value(QLatin1String("stopOnMetered")).toBool());
    const auto shouldRun = launcherSettingsObj.value(QLatin1String("run")).toBool();
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    auto options = LibSyncthing::RuntimeOptions();
    auto customSyncthingHome = launcherSettingsObj.value(QLatin1String("stHomeDir")).toString();
    m_syncthingConfigDir = customSyncthingHome.isEmpty() ? m_settingsDir->path() + QStringLiteral("/syncthing") : customSyncthingHome;
    m_syncthingDataDir = m_syncthingConfigDir;
    options.configDir = m_syncthingConfigDir.toStdString();
    options.dataDir = options.configDir;
    m_launcher.setLibSyncthingLogLevel(launcherSettingsObj.value(QLatin1String("logLevel")).toString());
    if (launcherSettingsObj.value(QLatin1String("writeLogFile")).toBool()) {
        if (!m_launcher.logFile().isOpen()) {
            m_launcher.logFile().setFileName(m_settingsDir->path() + QStringLiteral("/syncthing.log"));
            if (!m_launcher.logFile().open(QIODeviceBase::WriteOnly | QIODeviceBase::Append | QIODeviceBase::Text)) {
                emit error(tr("Unable to open persistent log file for Syncthing under \"%1\": %2")
                        .arg(m_launcher.logFile().fileName(), m_launcher.logFile().errorString()));
                return false;
            }
        }
    } else {
        m_launcher.logFile().close();
    }
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

bool AppService::applySettings()
{
    applySyncthingSettings();
    applyConnectionSettings(m_launcher.guiUrl());
    applyLauncherSettings();
    invalidateStatus();
    return true;
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
    const auto &icon = makeAndroidIcon(commonForkAwesomeIcons().exclamation);
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
    default:
        ;
    }
}
#endif

void AppService::handleConnectionError(
    const QString &errorMessage, Data::SyncthingErrorCategory category, int networkError, const QNetworkRequest &request, const QByteArray &response)
{
    Q_UNUSED(networkError)
    Q_UNUSED(category)
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
    emit logsAvailable(asText);
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

void AppService::updateAndroidNotification()
{
    const auto title = QJniObject::fromString(m_connection.isConnected() ? m_statusInfo.statusText() : status());
    const auto text = QJniObject::fromString(m_statusInfo.additionalStatusText());
    static const auto subText = QJniObject::fromString(QString());
    const auto &icon = makeAndroidIcon(m_statusInfo.statusIcon());
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
    const auto &icon = makeAndroidIcon(commonForkAwesomeIcons().exclamation);
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
    const auto &icon = makeAndroidIcon(commonForkAwesomeIcons().exclamation);
    updateExtraAndroidNotification(title, text, subText, page, icon, 3);
}

void AppService::showNewDevice(const QString &devId, const QString &message)
{
    const auto title = QJniObject::fromString(tr("Syncthing device wants to connect"));
    const auto text = QJniObject::fromString(message);
    static const auto subText = QJniObject::fromString(QString());
    const auto page = QJniObject::fromString(QStringLiteral("newdev:") + devId);
    const auto &icon = makeAndroidIcon(commonForkAwesomeIcons().networkWired);
    updateExtraAndroidNotification(title, text, subText, page, icon);
}

void AppService::showNewDir(const QString &devId, const QString &dirId, const QString &dirLabel, const QString &message)
{
    const auto title = QJniObject::fromString(tr("Syncthing device wants to share folder"));
    const auto text = QJniObject::fromString(message);
    static const auto subText = QJniObject::fromString(QString());
    const auto page = QJniObject::fromString(QStringLiteral("newfolder:") % devId % QChar(':') % dirId % QChar(':') % dirLabel);
    const auto &icon = makeAndroidIcon(commonForkAwesomeIcons().shareAlt);
    updateExtraAndroidNotification(title, text, subText, page, icon);
}
#endif

} // namespace QtGui
