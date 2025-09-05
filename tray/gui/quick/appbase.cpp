#include "./appbase.h"

#include <syncthingmodel/syncthingicons.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QJsonValue>
#include <QStandardPaths>

//#define SYNCTHINGTRAY_DEBUG_MAIN_LOOP_ACTIVITY

using namespace Data;

namespace QtGui {

/*!
 * \class AppBase
 * \brief The AppBase class implements common functionality of App and AppService.
 */

AppBase::AppBase(bool insecure, bool textOnly, bool clickToConnect, QObject *parent)
    : QObject(parent)
    , m_notifier(m_connection)
    , m_statusInfo(textOnly, clickToConnect)
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    , m_connectToLaunched(true)
#else
    , m_connectToLaunched(false)
#endif
    , m_insecure(insecure)
{
    qDebug() << "Initializing base";
#if defined(SYNCTHINGTRAY_DEBUG_MAIN_LOOP_ACTIVITY)
    if (auto *const app = QCoreApplication::instance()) {
        auto *const timer = new QTimer(this);
        QObject::connect(timer, &QTimer::timeout, app,
            [msg = QStringLiteral("%1 still active (%2)")
                    .arg(app->metaObject()->className(), qEnvironmentVariable("QT_QPA_PLATFORM", QStringLiteral("default")))] { qDebug() << msg; });
        timer->setInterval(1000);
        timer->start();
    }
#endif

    m_connection.setPollingFlags(SyncthingConnection::PollingFlags::MainEvents | SyncthingConnection::PollingFlags::Errors);
    m_connection.setAutoReconnectInterval(0); // avoid initial "Trying to reconnect …" status when using launcher but running Syncthing is disabled
    m_connectionSettingsFromLauncher.reconnectInterval = 0; // enable automatic reconnects only via handleGuiUrlChanged()
    m_connectionSettingsFromLauncher.statusComputionFlags += SyncthingStatusComputionFlags::RemoteSynchronizing;
    m_connectionSettingsFromConfig.statusComputionFlags += SyncthingStatusComputionFlags::RemoteSynchronizing;
#ifdef Q_OS_ANDROID
    m_notifier.setEnabledNotifications(
        SyncthingHighLevelNotification::ConnectedDisconnected | SyncthingHighLevelNotification::NewDevice | SyncthingHighLevelNotification::NewDir);
#else
    m_notifier.setEnabledNotifications(SyncthingHighLevelNotification::ConnectedDisconnected);
#endif
}

AppBase::~AppBase()
{
}

const QString &AppBase::status()
{
    if (m_status.has_value()) {
        return *m_status;
    }
    switch (m_connection.status()) {
    case Data::SyncthingStatus::Disconnected:
        return m_status.emplace(tr("Not connected to backend."));
    case Data::SyncthingStatus::Reconnecting:
        return m_status.emplace(tr("Waiting for backend …"));
    default:
        if (const auto errorCount = m_connection.errors().size()) {
            return m_status.emplace(tr("There are %n notification(s)/error(s).", nullptr, static_cast<int>(errorCount)));
        } else if (const auto internalErrorCount = m_internalErrors.size()) {
            return m_status.emplace(tr("There are %n Syncthing API error(s).", nullptr, static_cast<int>(internalErrorCount)));
        }
        return m_status.emplace();
    }
}

QString AppBase::openSettingFile(QFile &settingsFile, const QString &path)
{
    settingsFile.close();
    settingsFile.unsetError();
    settingsFile.setFileName(path);
    if (!settingsFile.open(QFile::ReadWrite)) {
        return tr("Unable to open settings under \"%1\": ").arg(path) + settingsFile.errorString();
    }
    return QString();
}

QDir &AppBase::settingsDir()
{
    if (!m_settingsDir.has_value()) {
        m_settingsDir.emplace(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    }
    return m_settingsDir.value();
}

bool AppBase::openSettings()
{
    auto &sd = settingsDir();
    if (!sd.exists() && !sd.mkpath(QStringLiteral("."))) {
        emit error(tr("Unable to create settings directory under \"%1\".").arg(sd.path()));
        m_settingsDir.reset();
        return false;
    }
    if (const auto errorMessage = openSettingFile(m_settingsFile, sd.path() + QStringLiteral("/appconfig.json")); !errorMessage.isEmpty()) {
        m_settingsDir.reset();
        emit error(errorMessage);
        return false;
    }
    return true;
}

void AppBase::invalidateStatus()
{
    m_status.reset();
    m_statusInfo.updateConnectionStatus(m_connection);
    m_statusInfo.updateConnectedDevices(m_connection);
}

Data::IconManager &AppBase::initIconManager()
{
    auto &iconManager = Data::IconManager::instance();
    auto statusIconSettings = StatusIconSettings();
    statusIconSettings.strokeWidth = StatusIconStrokeWidth::Thick;
    iconManager.applySettings(&statusIconSettings, &statusIconSettings, true, true);
    return iconManager;
}

QString AppBase::readSettingFile(QFile &settingsFile, QJsonObject &settings)
{
    auto settingsData = settingsFile.readAll();
    if (settingsFile.error() != QFile::NoError) {
        return tr("Unable to read settings: ") + settingsFile.errorString();
    }
    if (settingsData.isEmpty()) {
        return QString(); // settings file is expected to be empty on first startup
    }
    auto parsError = QJsonParseError();
    auto doc = QJsonDocument::fromJson(settingsData, &parsError);
    settings = doc.object();
    if (parsError.error != QJsonParseError::NoError || !doc.isObject()) {
        return tr("Unable to restore settings: ")
            + (parsError.error != QJsonParseError::NoError ? parsError.errorString() : tr("JSON document contains no object"));
    }
    return QString();
}

bool AppBase::loadSettings(bool force)
{
    if (force && m_settingsFile.isOpen()) {
        m_settingsFile.reset();
    }
    if (!m_settingsFile.isOpen() && !openSettings()) {
        return false;
    }
    if (const auto errorMessage = readSettingFile(m_settingsFile, m_settings); !errorMessage.isEmpty()) {
        emit error(errorMessage);
        return false;
    }
    emit settingsChanged(m_settings);
    return true;
}

void AppBase::applyConnectionSettings(const QUrl &syncthingUrl)
{
    auto connectionSettings = m_settings.value(QLatin1String("connection"));
    auto connectionSettingsObj = connectionSettings.toObject();
    auto couldLoadCertificate = false;
    auto modified = !connectionSettings.isObject();
    if (!modified) {
        couldLoadCertificate = m_connectionSettingsFromConfig.loadFromJson(connectionSettingsObj);
    } else {
        m_connectionSettingsFromConfig.storeToJson(connectionSettingsObj);
        couldLoadCertificate = m_connectionSettingsFromConfig.loadHttpsCert();
    }
    auto useLauncherVal = connectionSettingsObj.value(QStringLiteral("useLauncher"));
    m_connectToLaunched = useLauncherVal.toBool(m_connectToLaunched);
    if (!useLauncherVal.isBool()) {
        connectionSettingsObj.insert(QStringLiteral("useLauncher"), m_connectToLaunched);
        modified = true;
    }
    if (modified) {
        m_settings.insert(QLatin1String("connection"), connectionSettingsObj);
    }
    if (!couldLoadCertificate) {
        emit error(tr("Unable to load HTTPs certificate"));
    }
    m_connection.setInsecure(m_insecure);
    if (m_connectToLaunched) {
        handleGuiUrlChanged(syncthingUrl);
    } else if (m_connection.applySettings(m_connectionSettingsFromConfig) || !m_connection.isConnected()) {
        m_connection.reconnect();
    }
}

void AppBase::applySyncthingSettings()
{
    const auto launcherSettingsObj = m_settings.value(QLatin1String("launcher")).toObject();
    auto customSyncthingHome = launcherSettingsObj.value(QLatin1String("stHomeDir")).toString();
    m_syncthingConfigDir = customSyncthingHome.isEmpty() ? m_settingsDir->path() + QStringLiteral("/syncthing") : customSyncthingHome;
    m_syncthingDataDir = m_syncthingConfigDir;
}

void AppBase::handleGuiUrlChanged(const QUrl &newUrl)
{
    if (newUrl.scheme() == QLatin1String("unix") && !m_syncthingUnixSocketPath.isEmpty()) {
        m_connectionSettingsFromLauncher.syncthingUrl = QStringLiteral("unix+http://localhost");
        m_connectionSettingsFromLauncher.localPath = m_syncthingUnixSocketPath;
    } else {
#ifndef QT_NO_SSL
        auto url = newUrl;
        // always use TLS if supported by Qt for the sake of security (especially on Android)
        // note: Syncthing itself always supports it and allows connections via TLS even if the "tls" setting
        //       is disabled (because this setting is just about *enforcing* TLS).
        url.setScheme(QStringLiteral("https"));
#endif
        m_connectionSettingsFromLauncher.syncthingUrl = url.toString();
        m_connectionSettingsFromLauncher.localPath.clear();
    }
    if (!m_syncthingConfig.restore(m_syncthingConfigDir + QStringLiteral("/config.xml"))) {
        if (!newUrl.isEmpty()) {
            emit error("Unable to read Syncthing config for automatic connection to backend.");
        }
        return;
    }
    m_connectionSettingsFromLauncher.apiKey = m_syncthingConfig.guiApiKey.toUtf8();
    m_connectionSettingsFromLauncher.authEnabled = false;
    m_connectionSettingsFromLauncher.reconnectInterval = isSyncthingRunning() ? SyncthingConnectionSettings::defaultReconnectInterval : 0;
#ifndef QT_NO_SSL
    m_connectionSettingsFromLauncher.httpsCertPath = m_syncthingConfigDir + QStringLiteral("/https-cert.pem");
#endif

    if (m_connectToLaunched) {
        invalidateStatus();
        if (newUrl.isEmpty()) {
            m_connection.setAutoReconnectInterval(0);
            m_connection.disconnect();
        } else if (m_connection.applySettings(m_connectionSettingsFromLauncher) || !m_connection.isConnected()) {
            m_connection.reconnect();
        }
    }
}

} // namespace QtGui
