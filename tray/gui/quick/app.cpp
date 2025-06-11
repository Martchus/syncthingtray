#include "./app.h"

#include "resources/config.h"

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
#include <syncthing/interface.h>
#endif

#include <syncthingwidgets/misc/internalerror.h>
#include <syncthingwidgets/misc/otherdialogs.h>

#include <syncthingmodel/syncthingerrormodel.h>
#include <syncthingmodel/syncthingfilemodel.h>
#include <syncthingmodel/syncthingicons.h>

#include <syncthingconnector/syncthingconnectionsettings.h>
#include <syncthingconnector/utils.h>

#include <qtutilities/misc/desktoputils.h>
#include <qtutilities/resources/resources.h>

#include <qtforkawesome/renderer.h>
#include <qtquickforkawesome/imageprovider.h>

#include <c++utilities/conversion/stringbuilder.h>
#include <c++utilities/conversion/stringconversion.h>

#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonValue>
#include <QNetworkReply>
#include <QQmlContext>
#include <QQuickView>
#include <QStandardPaths>
#include <QStringBuilder>
#include <QUrlQuery>
#include <QtConcurrent/QtConcurrentRun>

#ifdef Q_OS_ANDROID
#include <QFontDatabase>
#include <QJniEnvironment>

#include <android/font.h>
#include <android/font_matcher.h>
#endif

#ifndef Q_OS_ANDROID
#include <QSet>
#include <QStorageInfo>
#endif

#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <numeric>
#include <stdexcept>

#ifdef Q_OS_WINDOWS
#define SYNCTHING_APP_STRING_CONVERSION(s) (s).toStdWString()
#define SYNCTHING_APP_PATH_CONVERSION(s) QString::fromStdWString((s).wstring())
#else
#define SYNCTHING_APP_STRING_CONVERSION(s) (s).toLocal8Bit().toStdString()
#define SYNCTHING_APP_PATH_CONVERSION(s) QString::fromLocal8Bit((s).string())
#endif

// configure dark mode depending on the platform
// 1. Some platforms just provide a "dark mode flag", e.g. Windows and Android. Qt can read this flag and
//    provide a Qt::ColorScheme value. Qt will only populate an appropriate QPalette on some platforms, e.g.
//    it does on Windows but not on Android. On platforms where Qt does not populate an appropriate palette
//    we therefore need to go by the Qt::ColorScheme value and populate the QPalette ourselves from the colors
//    used by the Qt Quick Controls 2 style. This behavior is enabled via SYNCTHING_APP_DARK_MODE_FROM_COLOR_SCHEME
//    in subsequent code.
//    Custom icons (Syncthing icons, ForkAwesome icons) are rendered using the text color from the application
//    QPalette. This is the reason why we still populate the QPalette in this case and don't just ignore it.
// 2. Some platforms allow the user to configure a custom palette but do *not* provide a "dark mode flag", e.g.
//    KDE. In this case reading the Qt::ColorScheme value from Qt is useless but QPalette will be populated. We
//    therefore need to determine whether the current color scheme is dark from the QPalette and set the Qt
//    Quick Controls 2 style based on that.
#ifdef Q_OS_ANDROID
#define SYNCTHING_APP_DARK_MODE_FROM_COLOR_SCHEME
#endif
#ifdef SYNCTHING_APP_DARK_MODE_FROM_COLOR_SCHEME
#define SYNCTHING_APP_IS_PALETTE_DARK(palette) false
#else
#define SYNCTHING_APP_IS_PALETTE_DARK(palette) QtUtilities::isPaletteDark(palette)
#endif

#define SYNCTHINGTRAY_DEBUG_MAIN_LOOP_ACTIVITY

using namespace Data;

namespace QtGui {

#ifdef Q_OS_ANDROID
SyncthingServiceBinder::SyncthingServiceBinder(AppService &service)
    : m_service(service)
{
    qDebug() << "Initializing Syncthing service binder";
}

bool SyncthingServiceBinder::onTransact(int code, const QAndroidParcel &data, const QAndroidParcel &reply, QAndroidBinder::CallType flags)
{
    Q_UNUSED(data)
    Q_UNUSED(reply)
    Q_UNUSED(flags)
    switch (code) {
    case SyncthingServiceBinder::ReloadSettings:
        QMetaObject::invokeMethod(&m_service, "reloadSettings", Qt::QueuedConnection);
        return true;
    default:
        return false;
    }
}

SyncthingServiceConnection::SyncthingServiceConnection()
{
}

bool SyncthingServiceConnection::connect()
{
    const auto serviceIntent = QAndroidIntent(QNativeInterface::QAndroidApplication::context(), "io.github.martchus.syncthingtray.SyncthingService");
    const auto bindResult = QtAndroidPrivate::bindService(serviceIntent, *this, QtAndroidPrivate::BindFlag::AutoCreate);
    qDebug() << "Binding to service: " << bindResult;
    return bindResult;
}

void SyncthingServiceConnection::onServiceConnected(const QString &name, const QAndroidBinder &serviceBinder)
{
    qDebug() << "Connected to service: " << name;
    m_binder = serviceBinder;
}

void SyncthingServiceConnection::onServiceDisconnected(const QString &name)
{
    qDebug() << "Disconnected from service: " << name;
    m_binder = QAndroidBinder();
}

/// \brief The JniFn namespace defines functions called from Java.
namespace JniFn {
static AppService *appServiceObjectForJava = nullptr;
static App *appObjectForJava = nullptr;

static void loadQtQuickGui(JNIEnv *, jobject)
{
    QMetaObject::invokeMethod(appObjectForJava, "initEngine", Qt::QueuedConnection);
}

static void unloadQtQuickGui(JNIEnv *, jobject)
{
    QMetaObject::invokeMethod(appObjectForJava, "destroyEngine", Qt::QueuedConnection);
}

static void stopLibSyncthing(JNIEnv *, jobject)
{
    QMetaObject::invokeMethod(appServiceObjectForJava, "stopLibSyncthing", Qt::QueuedConnection);
}

static void broadcastLauncherStatus(JNIEnv *, jobject)
{
    QMetaObject::invokeMethod(appServiceObjectForJava, "broadcastLauncherStatus", Qt::QueuedConnection);
}

static void handleLauncherStatusBroadcast(JNIEnv *, jobject, jobject intent)
{
    const auto status = QAndroidIntent(QJniObject::fromLocalRef(intent)).extraVariant(QStringLiteral("status"));
    QMetaObject::invokeMethod(appObjectForJava, "handleLauncherStatusBroadcast", Qt::QueuedConnection, Q_ARG(QVariant, status));
}

static void handleAndroidIntent(JNIEnv *, jobject, jstring page, jboolean fromNotification)
{
    QMetaObject::invokeMethod(appObjectForJava, "handleAndroidIntent", Qt::QueuedConnection,
        Q_ARG(QString, QJniObject::fromLocalRef(page).toString()), Q_ARG(bool, fromNotification));
}

static void handleStoragePermissionChanged(JNIEnv *, jobject, jboolean storagePermissionGranted)
{
    QMetaObject::invokeMethod(appObjectForJava, "handleStoragePermissionChanged", Qt::QueuedConnection, Q_ARG(bool, storagePermissionGranted));
}

static void handleNotificationPermissionChanged(JNIEnv *, jobject, jboolean notificationPermissionGranted)
{
    QMetaObject::invokeMethod(
        appObjectForJava, "handleNotificationPermissionChanged", Qt::QueuedConnection, Q_ARG(bool, notificationPermissionGranted));
}
} // namespace JniFn
#endif

/// \cond
static void ensureDefault(bool &mod, QJsonObject &o, QLatin1String member, const QJsonValue &d)
{
    if (!o.contains(member)) {
        o.insert(member, d);
        mod = true;
    }
}
/// \endcond

AppBase::AppBase(bool insecure, QObject *parent)
    : QObject(parent)
    , m_notifier(m_connection)
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
                    .arg(app->metaObject()->className(), qEnvironmentVariable("QT_QPA_PLATFORM", QStringLiteral("android")))] { qDebug() << msg; });
        timer->setInterval(1000);
        timer->start();
    }
#endif

    m_connection.setPollingFlags(SyncthingConnection::PollingFlags::MainEvents | SyncthingConnection::PollingFlags::Errors);
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

bool AppBase::openSettings()
{
    if (!m_settingsDir.has_value()) {
        m_settingsDir.emplace(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    }
    if (!m_settingsDir->exists() && !m_settingsDir->mkpath(QStringLiteral("."))) {
        emit error(tr("Unable to create settings directory under \"%1\".").arg(m_settingsDir->path()));
        m_settingsDir.reset();
        return false;
    }
    if (const auto errorMessage = openSettingFile(m_settingsFile, m_settingsDir->path() + QStringLiteral("/appconfig.json"));
        !errorMessage.isEmpty()) {
        m_settingsDir.reset();
        emit error(errorMessage);
        return false;
    }
    return true;
}

void AppBase::invalidateStatus()
{
    m_statusInfo.updateConnectionStatus(m_connection);
    m_statusInfo.updateConnectedDevices(m_connection);
}

QString AppBase::readSettingFile(QFile &settingsFile, QJsonObject &settings)
{
    auto parsError = QJsonParseError();
    auto doc = QJsonDocument::fromJson(settingsFile.readAll(), &parsError);
    settings = doc.object();
    if (settingsFile.error() != QFile::NoError) {
        return tr("Unable to read settings: ") + settingsFile.errorString();
    }
    if (parsError.error != QJsonParseError::NoError || !doc.isObject()) {
        return tr("Unable to restore settings: ")
            + (parsError.error != QJsonParseError::NoError ? parsError.errorString() : tr("JSON document contains no object"));
    }
    return QString();
}

bool AppBase::loadSettings()
{
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
        handleGuiAddressChanged(syncthingUrl);
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

void AppBase::handleGuiAddressChanged(const QUrl &newUrl)
{
    auto url = newUrl;
#ifndef QT_NO_SSL
    // always use TLS if supported by Qt for the sake of security (especially on Android)
    // note: Syncthing itself always supports it and allows connections via TLS even if the "tls" setting
    //       is disabled (because this setting is just about *enforcing* TLS).
    url.setScheme(QStringLiteral("https"));
#endif
    m_connectionSettingsFromLauncher.syncthingUrl = url.toString();
    if (!m_syncthingConfig.restore(m_syncthingConfigDir + QStringLiteral("/config.xml"))) {
        if (!newUrl.isEmpty()) {
            emit error("Unable to read Syncthing config for automatic connection to backend.");
        }
        return;
    }
    m_connectionSettingsFromLauncher.apiKey = m_syncthingConfig.guiApiKey.toUtf8();
    m_connectionSettingsFromLauncher.authEnabled = false;
#ifndef QT_NO_SSL
    m_connectionSettingsFromLauncher.httpsCertPath = m_syncthingConfigDir + QStringLiteral("/https-cert.pem");
#endif

    if (m_connectToLaunched) {
        invalidateStatus();
        if (newUrl.isEmpty()) {
            m_connection.disconnect();
        } else if (m_connection.applySettings(m_connectionSettingsFromLauncher) || !m_connection.isConnected()) {
            m_connection.reconnect();
        }
    }
}

AppService::AppService(bool insecure, QObject *parent)
    : AppBase(insecure, parent)
{
    qDebug() << "Initializing service app";

#ifdef Q_OS_ANDROID
    if (!JniFn::appServiceObjectForJava) {
        qDebug() << "Registering service JNI methods";
        JniFn::appServiceObjectForJava = this;
        auto env = QJniEnvironment();
        auto registeredMethods = true;
        static const JNINativeMethod activityMethods[] = {
            { "stopLibSyncthing", "()V", reinterpret_cast<void *>(JniFn::stopLibSyncthing) },
            { "broadcastLauncherStatus", "()V", reinterpret_cast<void *>(JniFn::broadcastLauncherStatus) },
        };
        registeredMethods = env.registerNativeMethods("io/github/martchus/syncthingtray/SyncthingService", activityMethods, 2) && registeredMethods;
        if (!registeredMethods) {
            qWarning() << "Unable to register all native methods in JNI environment.";
        }
    }
#endif

    qDebug() << "Initializing icon manager for service";
    auto &iconManager = Data::IconManager::instance();
    auto statusIconSettings = StatusIconSettings();
    statusIconSettings.strokeWidth = StatusIconStrokeWidth::Thick;
    iconManager.applySettings(&statusIconSettings, &statusIconSettings, true, true);
#ifdef Q_OS_ANDROID
    connect(&iconManager, &Data::IconManager::statusIconsChanged, this, &AppService::invalidateAndroidIconCache);
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
    connect(&m_launcher, &SyncthingLauncher::guiUrlChanged, this, &AppService::handleGuiAddressChanged);
    connect(&m_launcher, &SyncthingLauncher::guiUrlChanged, this, &AppService::broadcastLauncherStatus);

#ifdef Q_OS_ANDROID
    connect(&m_notifier, &SyncthingNotifier::newDevice, this, &AppService::showNewDevice);
    connect(&m_notifier, &SyncthingNotifier::newDir, this, &AppService::showNewDir);
#endif

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
    if (JniFn::appServiceObjectForJava == this) {
        JniFn::appServiceObjectForJava = nullptr;
    }
#endif
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
                // FIXME: emit error
                //emit error(tr("Unable to open persistent log file for Syncthing under \"%1\": %2")
                //        .arg(m_launcher.logFile().fileName(), m_launcher.logFile().errorString()));
                return false;
            }
        }
    } else {
        m_launcher.logFile().close();
    }
    m_launcher.setRunning(shouldRun, std::move(options));
#else
    if (shouldRun) {
        // FIXME: show error like in UI
        //emit error(tr("This build of the app cannot launch Syncthing."));
        return false;
    }
#endif
    return true;
}


bool AppService::reloadSettings()
{
    const auto res = loadSettings() && applyLauncherSettings();
    invalidateStatus();
    return res;
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
void AppService::stopLibSyncthing()
{
    m_launcher.stopLibSyncthing();
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
    // FIXME: const auto title = QJniObject::fromString(m_connection.isConnected() ? m_statusInfo.statusText() : status());
    const auto title = QJniObject::fromString(m_statusInfo.statusText());
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

App::App(bool insecure, QObject *parent)
    : AppBase(insecure, parent)
    , m_app(static_cast<QGuiApplication *>(QCoreApplication::instance()))
    , m_imageProvider(nullptr)
    , m_dirModel(m_connection)
    , m_sortFilterDirModel(&m_dirModel)
    , m_devModel(m_connection)
    , m_sortFilterDevModel(&m_devModel)
    , m_changesModel(m_connection)
    , m_faUrlBase(QStringLiteral("image://fa/"))
    , m_uiObjects({ &m_dirModel, &m_sortFilterDirModel, &m_devModel, &m_sortFilterDevModel, &m_changesModel })
    , m_iconSize(16)
    , m_tabIndex(-1)
    , m_importExportStatus(ImportExportStatus::None)
    , m_darkmodeEnabled(false)
    , m_darkColorScheme(false)
    , m_darkPalette(m_app ? SYNCTHING_APP_IS_PALETTE_DARK(m_app->palette()) : false)
    , m_isGuiLoaded(false)
    , m_alwaysUnloadGuiWhenHidden(false)
    , m_unloadGuiWhenHidden(false)
{
    qDebug() << "Initializing UI";

#ifdef Q_OS_ANDROID
    m_serviceConnection.connect();
#endif

    m_app->installEventFilter(this);
    m_app->setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
    connect(m_app, &QGuiApplication::applicationStateChanged, this, &App::handleStateChanged);

    QtUtilities::deletePipelineCacheIfNeeded();
    loadSettings();
    applySettings();
    if (m_app) {
#ifdef SYNCTHING_APP_DARK_MODE_FROM_COLOR_SCHEME
        QtUtilities::onDarkModeChanged([this](bool darkColorScheme) { applyDarkmodeChange(darkColorScheme, m_darkPalette); }, this);
#else
        applyDarkmodeChange(m_darkColorScheme, m_darkPalette);
#endif
    }

    qDebug() << "Initializing icon manager";
    auto &iconManager = Data::IconManager::instance();
    auto statusIconSettings = StatusIconSettings();
    statusIconSettings.strokeWidth = StatusIconStrokeWidth::Thick;
    iconManager.applySettings(&statusIconSettings, &statusIconSettings, true, true);
    connect(&iconManager, &Data::IconManager::statusIconsChanged, this, &App::statusInfoChanged);

    // set SD card paths to show SD card icons via directory model
#ifdef Q_OS_ANDROID
    qDebug() << "Loading external storage paths";
    auto extStoragePaths = externalStoragePaths();
    auto sdCardPaths = QStringList();
    if (extStoragePaths.size() > 1) {
        static constexpr auto storagePrefix = QLatin1String("/storage/");
        sdCardPaths.reserve(extStoragePaths.size() - 1);
        for (auto i = extStoragePaths.begin() + 1, end = extStoragePaths.end(); i != end; ++i) {
            if (const auto &path = *i; path.startsWith(storagePrefix)) {
                if (const auto volumeEnd = path.indexOf(QChar('/'), storagePrefix.size()); volumeEnd > 0) {
                    sdCardPaths.emplace_back(QStringView(path.data(), volumeEnd + 1));
                }
            }
        }
    }
    m_dirModel.setSdCardPaths(sdCardPaths);
#endif

    m_sortFilterDirModel.sort(0, Qt::AscendingOrder);
    m_sortFilterDevModel.sort(0, Qt::AscendingOrder);

    connect(&m_connection, &SyncthingConnection::error, this, &App::handleConnectionError);
    connect(&m_connection, &SyncthingConnection::statusChanged, this, &App::handleConnectionStatusChanged);
    connect(&m_connection, &SyncthingConnection::newDevices, this, &App::handleChangedDevices);
    connect(&m_connection, &SyncthingConnection::autoReconnectIntervalChanged, this, &App::invalidateStatus);
    connect(&m_connection, &SyncthingConnection::hasOutOfSyncDirsChanged, this, &App::invalidateStatus);
    connect(&m_connection, &SyncthingConnection::devStatusChanged, this, &App::handleChangedDevices);
    connect(&m_connection, &SyncthingConnection::newErrors, this, &App::handleNewErrors);

    // show info when override/revert has been triggered
    connect(&m_connection, &SyncthingConnection::overrideTriggered, this,
        [this](const QString &dirId) { emit info(tr("Triggered override of \"%1\"").arg(dirDisplayName(dirId))); });
    connect(&m_connection, &SyncthingConnection::revertTriggered, this,
        [this](const QString &dirId) { emit info(tr("Triggered revert of \"%1\"").arg(dirDisplayName(dirId))); });

#ifdef Q_OS_ANDROID
    // register native methods of Android activity
    if (!JniFn::appObjectForJava) {
        qDebug() << "Registering UI JNI methods";
        JniFn::appObjectForJava = this;
        auto env = QJniEnvironment();
        auto registeredMethods = true;
        static const JNINativeMethod activityMethods[] = {
            { "handleLauncherStatusBroadcast", "(Landroid/content/Intent;)V", reinterpret_cast<void *>(JniFn::handleLauncherStatusBroadcast) },
            { "handleAndroidIntent", "(Ljava/lang/String;Z)V", reinterpret_cast<void *>(JniFn::handleAndroidIntent) },
            { "handleStoragePermissionChanged", "(Z)V", reinterpret_cast<void *>(JniFn::handleStoragePermissionChanged) },
            { "handleNotificationPermissionChanged", "(Z)V", reinterpret_cast<void *>(JniFn::handleNotificationPermissionChanged) },
            { "loadQtQuickGui", "()V", reinterpret_cast<void *>(JniFn::loadQtQuickGui) },
            { "unloadQtQuickGui", "()V", reinterpret_cast<void *>(JniFn::unloadQtQuickGui) },
        };
        registeredMethods = env.registerNativeMethods("io/github/martchus/syncthingtray/Activity", activityMethods, 6) && registeredMethods;
        if (!registeredMethods) {
            qWarning() << "Unable to register all native methods in JNI environment.";
        }
    }

    // start Android service
    qDebug() << "Starting Android service";
    QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("startSyncthingService");
#endif

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &App::shutdown);

    initEngine();

    qDebug() << "App initialized";
}

App::~App()
{
    qDebug() << "Destorying app";
#ifdef Q_OS_ANDROID
    if (JniFn::appObjectForJava == this) {
        JniFn::appObjectForJava = nullptr;
    }
#endif
}

App *App::create(QQmlEngine *qmlEngine, QJSEngine *engine)
{
    auto *const app = engine->property("app").value<App *>();
    if (qmlEngine) {
        auto *const imageProvider = new QtForkAwesome::QuickImageProvider(Data::IconManager::instance().forkAwesomeRenderer());
        connect(app->m_imageProvider = imageProvider, &QObject::destroyed, app, [app]() { app->m_imageProvider = nullptr; });
        qmlEngine->addImageProvider(QStringLiteral("fa"), imageProvider);
    }
    QJSEngine::setObjectOwnership(app, QJSEngine::CppOwnership);
    return app;
}

QString App::website() const
{
    return QStringLiteral(APP_URL);
}

const QString &App::status()
{
    if (m_status.has_value()) {
        return *m_status;
    }
    switch (m_importExportStatus) {
    case ImportExportStatus::None:
        break;
    case ImportExportStatus::Checking:
        return m_status.emplace(tr("Checking for data to import …"));
    case ImportExportStatus::Importing:
        return m_status.emplace(tr("Importing configuration …"));
    case ImportExportStatus::Exporting:
        return m_status.emplace(tr("Exporting configuration …"));
    case ImportExportStatus::CheckingMove:
        return m_status.emplace(tr("Checking locations to move home directory …"));
    case ImportExportStatus::Moving:
        return m_status.emplace(tr("Moving home directory …"));
    }
    if (m_connectToLaunched) {
        if (!m_launcherStatus.value(QStringLiteral("isRunning")).toBool()) {
            return m_status.emplace(m_launcherStatus.value(QStringLiteral("runningStatus")).toString());
        } else if (m_launcherStatus.value(QStringLiteral("isStarting")).toBool()) {
            return m_status.emplace(tr("Backend is starting …"));
        }
    }
    switch (m_connection.status()) {
    case Data::SyncthingStatus::Disconnected:
        return m_status.emplace(tr("Not connected to backend."));
    case Data::SyncthingStatus::Reconnecting:
        return m_status.emplace(tr("Waiting for backend …"));
    default:
        if (m_pendingConfigChange.reply) {
            return m_status.emplace(tr("Saving configuration …"));
        } else if (const auto errorCount = m_connection.errors().size()) {
            return m_status.emplace(tr("There are %n notification(s)/error(s).", nullptr, static_cast<int>(errorCount)));
        } else if (const auto internalErrorCount = m_internalErrors.size()) {
            return m_status.emplace(tr("There are %n Syncthing API error(s).", nullptr, static_cast<int>(internalErrorCount)));
        }
        return m_status.emplace();
    }
}

qint64 App::databaseSize(const QString &path, const QString &extension) const
{
    const auto dir = QDir(m_syncthingDataDir % QChar('/') % path);
    const auto files = dir.entryInfoList({ extension }, QDir::Files);
    return std::accumulate(files.begin(), files.end(), qint64(), [](auto size, const auto &file) { return size + file.size(); });
}

QVariant App::formattedDatabaseSize(const QString &path, const QString &extension) const
{
    const auto size = databaseSize(path, extension);
    return size > 0 ? QVariant(QString::fromStdString(CppUtilities::dataSizeToString(static_cast<std::uint64_t>(size)))) : QVariant();
}

QVariantMap App::statistics() const
{
    auto stats = QVariantMap();
    statistics(stats);
    return stats;
}

void App::statistics(QVariantMap &res) const
{

    res[QStringLiteral("stConfigDir")] = m_syncthingConfigDir;
    res[QStringLiteral("stDataDir")] = m_syncthingDataDir;
    res[QStringLiteral("stLevelDbSize")] = formattedDatabaseSize(QStringLiteral("index-v0.14.0.db"), QStringLiteral("*.ldb"));
    res[QStringLiteral("stLevelDbMigratedSize")] = formattedDatabaseSize(QStringLiteral("index-v0.14.0.db-migrated"), QStringLiteral("*.ldb"));
    res[QStringLiteral("stSQLiteDbSize")] = formattedDatabaseSize(QStringLiteral("index-v2"), QStringLiteral("*.db*"));
#ifdef Q_OS_ANDROID
    res[QStringLiteral("extFilesDir")] = externalFilesDir();
    res[QStringLiteral("extStoragePaths")] = externalStoragePaths();
#endif
}

#if !(defined(Q_OS_ANDROID) || defined(Q_OS_WINDOWS))
bool App::nativePopups() const
{
    static const auto enableNativePopups = [] {
        auto ok = false;
        return qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_NATIVE_POPUPS", &ok) > 0 || !ok;
    }();
    return enableNativePopups;
}
#endif

bool App::initEngine()
{
    qDebug() << "Initializing QML engine";
    m_engine.emplace();
    connect(
        &*m_engine, &QQmlApplicationEngine::objectCreated, m_app,
        [](QObject *obj, const QUrl &objUrl) {
            if (!obj) {
                std::cerr << "Unable to load " << objUrl.toString().toStdString() << '\n';
                QCoreApplication::exit(EXIT_FAILURE);
            }
        },
        Qt::QueuedConnection);
    connect(&*m_engine, &QQmlApplicationEngine::quit, m_app, &QGuiApplication::quit);
    m_engine->setProperty("app", QVariant::fromValue(this));
    return loadMain();
}

#ifdef Q_OS_ANDROID
bool App::destroyEngine()
{
    unloadMain();
    qDebug() << "Destroying QML engine";
    m_engine.reset();
    return true;
}
#endif

bool App::loadMain()
{
    // allow overriding Qml entry point for hot-reloading; otherwise load proper Qml module from resources
    qDebug() << "Loading Qt Quick GUI";
    if (const auto path = qEnvironmentVariable(PROJECT_VARNAME_UPPER "_QML_ENTRY_POINT_PATH"); !path.isEmpty()) {
        qDebug() << "Path Qml entry point for Qt Quick GUI was overridden to: " << path;
        m_engine->load(path);
    } else if (const auto type = qEnvironmentVariable(PROJECT_VARNAME_UPPER "_QML_ENTRY_POINT_TYPE"); !type.isEmpty()) {
        m_engine->loadFromModule("Main", type);
        if (const auto rootObjects = m_engine->rootObjects(); !rootObjects.isEmpty()) {
            auto *const window = new QQuickView(&m_engine.value(), nullptr);
            window->loadFromModule("Main", type);
            window->setTitle(type);
            window->setWidth(700);
            window->setHeight(500);
            window->setVisible(true);
        }
    } else {
        m_engine->loadFromModule("Main", "AppWindow");
    }
    m_isGuiLoaded = true;
    return true;
}

bool App::reloadMain()
{
    qDebug() << "Reloading Qt Quick GUI";
    unloadMain();
    m_engine->clearComponentCache();
    return loadMain();
}

bool App::unloadMain()
{
    qDebug() << "Unloading Qt Quick GUI";
    const auto rootObjects = m_engine->rootObjects();
    for (auto *const rootObject : rootObjects) {
        rootObject->deleteLater();
    }
    m_isGuiLoaded = false;
    return true;
}

void App::shutdown()
{
    // FIXME: stop libsyncthing
    //m_launcher.stopLibSyncthing();
#ifdef Q_OS_ANDROID
    QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("stopSyncthingService");
#endif
}

bool App::openPath(const QString &path)
{
    if (QtUtilities::openLocalFileOrDir(path)) {
        return true;
    }
    emit error(tr("Unable to open \"%1\"").arg(path));
    return false;
}

bool App::openPath(const QString &dirId, const QString &relativePath)
{
    auto row = int();
    auto dirInfo = m_connection.findDirInfo(dirId, row);
    if (!dirInfo) {
        emit error(tr("Unable to open \"%1\"").arg(relativePath));
    } else if (openPath(dirInfo->path % QChar('/') % relativePath)) {
        return true;
    }
    return false;
}

bool App::scanPath(const QString &path)
{
#ifdef Q_OS_ANDROID
    if (QJniObject(QNativeInterface::QAndroidApplication::context())
            .callMethod<jboolean>("scanPath", "(Ljava/lang/String;)Z", QJniObject::fromString(path))) {
        return true;
    }
#else
    Q_UNUSED(path)
#endif
    emit error(tr("Scanning is not supported."));
    return false;
}

bool App::copyText(const QString &text)
{
    if (auto *const clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(text);
#ifdef Q_OS_ANDROID
        // do not emit info as Android will show a message about it automatically
        performHapticFeedback();
#else
        emit info(tr("Copied value"));
#endif
        return true;
    }
    emit info(tr("Unable to copy value"));
    return false;
}

bool App::copyPath(const QString &dirId, const QString &relativePath)
{
    auto row = int();
    auto dirInfo = m_connection.findDirInfo(dirId, row);
    if (!dirInfo) {
        emit error(tr("Unable to copy \"%1\"").arg(relativePath));
    } else if (copyText(dirInfo->path % QChar('/') % relativePath)) {
        return true;
    }
    return false;
}

QString App::getClipboardText() const
{
    if (auto *const clipboard = QGuiApplication::clipboard()) {
        return clipboard->text();
    }
    return QString();
}

bool App::loadIgnorePatterns(const QString &dirId, QObject *textArea)
{
    auto res = m_connection.ignores(dirId, [this, textArea](SyncthingIgnores &&ignores, QString &&error) {
        if (!error.isEmpty()) {
            emit this->error(tr("Unable to load ignore patterns: ") + error);
        }
        textArea->setProperty("text", ignores.ignore.join(QChar('\n')));
        textArea->setProperty("enabled", true);
    });
    connect(textArea, &QObject::destroyed, res.reply, &QNetworkReply::deleteLater);
    connect(this, &QObject::destroyed, [connection = res.connection] { disconnect(connection); });
    return true;
}

bool App::saveIgnorePatterns(const QString &dirId, QObject *textArea)
{
    textArea->setProperty("enabled", false);
    const auto text = textArea->property("text");
    if (text.userType() != QMetaType::QString) {
        textArea->setProperty("enabled", true);
        return false;
    }
    auto res = m_connection.setIgnores(
        dirId, SyncthingIgnores{ .ignore = text.toString().split(QChar('\n')), .expanded = QStringList() }, [this, textArea](QString &&error) {
            if (!error.isEmpty()) {
                emit this->error(tr("Unable to save ignore patterns: ") + error);
            }
            textArea->setProperty("enabled", true);
        });
    connect(textArea, &QObject::destroyed, res.reply, &QNetworkReply::deleteLater);
    connect(this, &QObject::destroyed, [connection = res.connection] { disconnect(connection); });
    return true;
}

bool App::openIgnorePatterns(const QString &dirId)
{
    return openPath(dirId, QStringLiteral(".stignore"));
}

bool App::loadErrors(QObject *listView)
{
    listView->setProperty("model", QVariant::fromValue(new Data::SyncthingErrorModel(m_connection, listView)));
    return true;
}

bool App::showLog(QObject *textArea)
{
    textArea->setProperty("text", m_log);
    connect(this, &App::logsAvailable, textArea, [textArea](const QString &newLogs) {
        QMetaObject::invokeMethod(textArea, "insert", Q_ARG(int, textArea->property("length").toInt()), Q_ARG(QString, newLogs));
    });
    return true;
}

bool App::showQrCode(Icon *icon)
{
    if (m_connection.myId().isEmpty()) {
        return false;
    }
    connect(&m_connection, &Data::SyncthingConnection::qrCodeAvailable, icon,
        [icon, requestedId = m_connection.myId(), this](const QString &id, const QByteArray &data) {
            if (id == requestedId) {
                disconnect(&m_connection, nullptr, icon, nullptr);
                icon->setSource(QImage::fromData(data));
            }
        });
    m_connection.requestQrCode(m_connection.myId());
    return true;
}

bool App::loadDirErrors(const QString &dirId, QObject *view)
{
    auto connection = connect(&m_connection, &Data::SyncthingConnection::dirStatusChanged, view, [this, dirId, view](const Data::SyncthingDir &dir) {
        if (dir.id != dirId) {
            return;
        }
        auto array = m_engine->newArray(static_cast<quint32>(dir.itemErrors.size()));
        auto index = quint32();
        for (const auto &itemError : dir.itemErrors) {
            auto error = m_engine->newObject();
            error.setProperty(QStringLiteral("path"), itemError.path);
            error.setProperty(QStringLiteral("message"), itemError.message);
            array.setProperty(index++, error);
        }
        view->setProperty("model", array.toVariant());
        view->setProperty("enabled", true);
    });
    connect(this, &QObject::destroyed, [connection] { disconnect(connection); });
    m_connection.requestDirPullErrors(dirId);
    return true;
}

bool App::loadStatistics(const QJSValue &callback)
{
    if (!callback.isCallable()) {
        return false;
    }
    auto query = m_connection.requestJsonData(
        QByteArrayLiteral("GET"), QStringLiteral("svc/report"), QUrlQuery(), QByteArray(), [this, callback](QJsonDocument &&doc, QString &&error) {
            auto report = doc.object().toVariantMap();
            report[QStringLiteral("uptime")] = QString::fromStdString(CppUtilities::TimeSpan::fromSeconds(report[QStringLiteral("uptime")].toDouble())
                    .toString(CppUtilities::TimeSpanOutputFormat::WithMeasures));
            report[QStringLiteral("memoryUsage")] = QString::fromStdString(
                CppUtilities::dataSizeToString(static_cast<std::uint64_t>(report.take(QStringLiteral("memoryUsageMiB")).toDouble() * 1024 * 1024)));
            report[QStringLiteral("processRSS")] = QString::fromStdString(
                CppUtilities::dataSizeToString(static_cast<std::uint64_t>(report.take(QStringLiteral("processRSSMiB")).toDouble() * 1024 * 1024)));
            report[QStringLiteral("memorySize")] = QString::fromStdString(
                CppUtilities::dataSizeToString(static_cast<std::uint64_t>(report[QStringLiteral("memorySize")].toDouble() * 1024 * 1024)));
            report[QStringLiteral("totSize")] = QString::fromStdString(
                CppUtilities::dataSizeToString(static_cast<std::uint64_t>(report.take(QStringLiteral("totMiB")).toDouble() * 1024 * 1024)));
            statistics(report);
            callback.call(QJSValueList({ m_engine->toScriptValue(report), QJSValue(std::move(error)) }));
        });
    connect(this, &QObject::destroyed, query.reply, &QNetworkReply::deleteLater);
    connect(this, &QObject::destroyed, [c = query.connection] { disconnect(c); });
    return true;
}

bool App::showError(const QString &errorMessage)
{
    emit error(errorMessage);
    return true;
}

Data::SyncthingFileModel *App::createFileModel(const QString &dirId, QObject *parent)
{
    auto row = int();
    auto dirInfo = m_connection.findDirInfo(dirId, row);
    if (!dirInfo) {
        return nullptr;
    }
    auto model = new Data::SyncthingFileModel(m_connection, *dirInfo, parent);
    connect(model, &Data::SyncthingFileModel::notification, this,
        [this](const QString &type, const QString &message, const QString &details = QString()) {
            type == QStringLiteral("error") ? emit error(message, details) : emit info(message, details);
        });
    return model;
}

DiffHighlighter *App::createDiffHighlighter(QTextDocument *parent)
{
    return new DiffHighlighter(parent);
}

#ifdef Q_OS_ANDROID
#define REQUIRES_ANDROID_API(x) __attribute__((__availability__(android, introduced = x)))
#define ANDROID_API_AT_LEAST(x) __builtin_available(android x, *)

static void loadSystemFont(std::string_view fontPath, QString &fontFamily)
{
    auto fontFile = QFile(QString::fromUtf8(fontPath.data(), static_cast<qsizetype>(fontPath.size())));
    if (!fontFile.open(QFile::ReadOnly)) {
        qDebug() << "Unable to open font file: " << fontPath;
        return;
    }
    if (const auto fontID = QFontDatabase::addApplicationFontFromData(fontFile.readAll()); fontID != -1) {
        if (const auto families = QFontDatabase::applicationFontFamilies(fontID); !families.isEmpty() && fontFamily.isEmpty()) {
            qDebug() << "Loaded font file: " << fontPath;
            fontFamily = families.front();
        }
    } else {
        qDebug() << "Unable to load font file: " << fontPath;
    }
}

static void matchAndLoadDefaultFont(AFontMatcher *matcher, std::uint16_t weight, QString &fontFamily) REQUIRES_ANDROID_API(34)
{
    AFontMatcher_setStyle(matcher, weight, false);
    if (auto *const font = AFontMatcher_match(matcher, "FontFamily.Default", reinterpret_cast<const uint16_t *>(u"foobar"), 3, nullptr)) {
        auto path = std::string_view(AFont_getFontFilePath(font));
        qDebug() << "Found system font: " << path;
        loadSystemFont(path, fontFamily);
        AFont_close(font);
    }
}

/*!
 * \brief Loads the default system font on Android 14 and newer.
 * \remarks The API would be available as of Android 10 (API 29) but using it leads to crashes. Not sure about
 *          Android 11, 12 and 13 so only enabling this as of Android 14.
 */
static QString loadDefaultSystemFonts()
{
    auto defaultSystemFontFamily = QString();
    if (ANDROID_API_AT_LEAST(34)) {
        if (auto *const m = AFontMatcher_create()) {
            matchAndLoadDefaultFont(m, AFONT_WEIGHT_NORMAL, defaultSystemFontFamily);
            matchAndLoadDefaultFont(m, AFONT_WEIGHT_THIN, defaultSystemFontFamily);
            matchAndLoadDefaultFont(m, AFONT_WEIGHT_LIGHT, defaultSystemFontFamily);
            matchAndLoadDefaultFont(m, AFONT_WEIGHT_MEDIUM, defaultSystemFontFamily);
            matchAndLoadDefaultFont(m, AFONT_WEIGHT_BOLD, defaultSystemFontFamily);
            AFontMatcher_destroy(m);
        }
    }
    return defaultSystemFontFamily;
}
#endif

QString App::fontFamily() const
{
#ifdef Q_OS_ANDROID
    static const auto defaultSystemFontFamily = loadDefaultSystemFonts();
    if (defaultSystemFontFamily.isEmpty()) {
        qDebug() << "Unable to determine/load system font family.";
    } else {
        qDebug() << "System font family: " << defaultSystemFontFamily;
        return defaultSystemFontFamily;
    }
#endif
    return QString();
}

qreal App::fontScale() const
{
#ifdef Q_OS_ANDROID
    return qreal(QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jfloat>("fontScale", "()F"));
#else
    return qreal(1.0);
#endif
}

int App::fontWeightAdjustment() const
{
#ifdef Q_OS_ANDROID
    return QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jint>("fontWeightAdjustment", "()I");
#else
    return 0;
#endif
}

/*!
 * \brief Return the font applying extra settings where necessary.
 * \remarks On some platforms like on Android this is required because Qt does not read these settings.
 */
QFont App::font() const
{
    auto f = QFont();
    if (const auto family = fontFamily(); !family.isEmpty()) {
        f.setFamily(family);
    }
    if (const auto scale = fontScale(); scale != qreal(1.0)) {
        f.setPixelSize(static_cast<int>(static_cast<qreal>(f.pixelSize()) * scale));
        f.setPointSizeF(f.pointSizeF() * scale);
    }
    if (const auto weightAdjustment = fontWeightAdjustment(); weightAdjustment != 0) {
        f.setWeight(static_cast<QFont::Weight>(f.weight() + weightAdjustment));
    }
    return f;
}

bool App::storagePermissionGranted() const
{
#ifdef Q_OS_ANDROID
    if (!m_storagePermissionGranted.has_value()) {
        m_storagePermissionGranted = QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jboolean>("storagePermissionGranted");
    }
    return m_storagePermissionGranted.value();
#else
    return true;
#endif
}

bool App::notificationPermissionGranted() const
{
#ifdef Q_OS_ANDROID
    if (!m_notificationPermissionGranted.has_value()) {
        m_notificationPermissionGranted
            = QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jboolean>("notificationPermissionGranted");
    }
    return m_notificationPermissionGranted.value();
#else
    return true;
#endif
}

QString App::externalFilesDir() const
{
#ifdef Q_OS_ANDROID
    return QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jstring>("externalFilesDir").toString();
#else
    return QString();
#endif
}

QStringList App::externalStoragePaths() const
{
    auto paths = QStringList();
#ifdef Q_OS_ANDROID
    auto env = QJniEnvironment();
    const auto pathArray
        = QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jobjectArray>("externalStoragePaths", "()[Ljava/lang/String;");
    const auto pathArrayObj = pathArray.object<jobjectArray>();
    const auto pathArrayLength = env->GetArrayLength(pathArrayObj);
    paths.reserve(pathArrayLength);
    for (int i = 0; i != pathArrayLength; ++i) {
        paths.append((QJniObject::fromLocalRef(env->GetObjectArrayElement(pathArrayObj, i)).toString()));
    }
#endif
    return paths;
}

bool App::requestStoragePermission()
{
#ifdef Q_OS_ANDROID
    return QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jboolean>("requestStoragePermission");
#else
    return false;
#endif
}

bool App::requestNotificationPermission()
{
#ifdef Q_OS_ANDROID
    return QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jboolean>("requestNotificationPermission");
#else
    return false;
#endif
}

void App::addDialog(QObject *dialog)
{
    m_dialogs.append(dialog);
    connect(dialog, &QObject::destroyed, this, &App::removeDialog);
}

void App::removeDialog(QObject *dialog)
{
    disconnect(dialog, &QObject::destroyed, this, &App::removeDialog);
    m_dialogs.removeAll(dialog);
}

bool App::eventFilter(QObject *object, QEvent *event)
{
    if (object != m_app) {
        return false;
    }
    switch (event->type()) {
    case QEvent::ApplicationPaletteChange:
        qDebug() << "Application palette has changed";
        if (m_app) {
            const auto palette = m_app->palette();
            IconManager::instance(&palette).setPalette(palette);
            if (m_imageProvider) {
                m_imageProvider->setDefaultColor(palette.color(QPalette::Normal, QPalette::Text));
            }
#ifndef SYNCTHING_APP_DARK_MODE_FROM_COLOR_SCHEME
            applyDarkmodeChange(m_darkColorScheme, SYNCTHING_APP_IS_PALETTE_DARK(palette));
#endif
        }
        break;
    default:;
    }
    return false;
}

void App::handleConnectionError(
    const QString &errorMessage, Data::SyncthingErrorCategory category, int networkError, const QNetworkRequest &request, const QByteArray &response)
{
    if (!InternalError::isRelevant(m_connection, category, errorMessage, networkError, false)) {
        return;
    }
    qWarning() << "Connection error: " << errorMessage;
    auto error = InternalError(errorMessage, request.url(), response);
    emit internalError(error);
    const auto emitHasInternalErrorsChanged = m_internalErrors.isEmpty();
    m_internalErrors.emplace_back(QVariant::fromValue(std::move(error)));
    if (emitHasInternalErrorsChanged) {
        invalidateStatus();
        emit hasInternalErrorsChanged();
    }
}

void App::setCurrentControls(bool visible, int tabIndex)
{
    auto flags = m_connection.pollingFlags();
    if (tabIndex < 0) {
        tabIndex = m_tabIndex;
    } else {
        m_tabIndex = tabIndex;
    }
    CppUtilities::modFlagEnum(flags, Data::SyncthingConnection::PollingFlags::TrafficStatistics, visible && tabIndex == 0);
    CppUtilities::modFlagEnum(flags, Data::SyncthingConnection::PollingFlags::DeviceStatistics, visible && tabIndex == 2);
    CppUtilities::modFlagEnum(flags, Data::SyncthingConnection::PollingFlags::DiskEvents, visible && tabIndex == 3);
    m_connection.setPollingFlags(flags);
}

bool App::performHapticFeedback()
{
#ifdef Q_OS_ANDROID
    return QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jboolean>("performHapticFeedback");
#else
    return false;
#endif
}

bool App::showToast(const QString &message)
{
#ifdef Q_OS_ANDROID
    return QJniObject(QNativeInterface::QAndroidApplication::context())
        .callMethod<jboolean>("showToast", "(Ljava/lang/String;)Z", QJniObject::fromString(message));
#else
    Q_UNUSED(message)
    return false;
#endif
}

QString App::resolveUrl(const QUrl &url)
{
#if defined(Q_OS_ANDROID)
    const auto urlString = url.toString(QUrl::FullyEncoded);
    const auto path = QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jstring>("resolveUri", urlString).toString();
    if (path.isEmpty()) {
        showToast(tr("Unable to resolve URL \"%1\".").arg(urlString));
    }
    return path.isEmpty() ? urlString : path;
#elif defined(Q_OS_WINDOWS)
    auto path = url.path();
    while (path.startsWith(QChar('/'))) {
        path.remove(0, 1);
    }
    return path;
#else
    return url.path();
#endif
}

bool App::shouldIgnorePermissions(const QString &path)
{
#ifdef Q_OS_ANDROID
    // QStorageInfo only returns "fuse" on Android but we can assume that permissions should be generally ignored
    Q_UNUSED(path)
    return true;
#else
    static const auto problematicFileSystems = QSet<QByteArray>({ QByteArrayLiteral("fat"), QByteArrayLiteral("vfat"), QByteArrayLiteral("exfat") });
    const auto storageInfo = QStorageInfo(path);
    return storageInfo.isValid() && problematicFileSystems.contains(storageInfo.fileSystemType());
#endif
}

void App::invalidateStatus()
{
    AppBase::invalidateStatus();
    m_status.reset();
    m_statusInfo.updateConnectionStatus(m_connection);
    m_statusInfo.updateConnectedDevices(m_connection);
    emit statusInfoChanged();
    emit statusChanged();
}

void App::gatherLogs(const QByteArray &newOutput)
{
    const auto asText = QString::fromUtf8(newOutput);
    emit logsAvailable(asText);
    m_log.append(asText);
}

void App::handleRunningChanged(bool isRunning)
{
    if (m_connectToLaunched) {
        invalidateStatus();
    }
    if (!m_settingsImport.first.isEmpty() && !isRunning) {
        importSettings(m_settingsImport.first, m_settingsImport.second);
    } else if (m_homeDirMove.has_value()) {
        moveSyncthingHome(m_homeDirMove.value());
    }
}

void App::handleChangedDevices()
{
    m_statusInfo.updateConnectedDevices(m_connection);
    emit statusInfoChanged();
}

void App::handleNewErrors(const std::vector<Data::SyncthingError> &errors)
{
    Q_UNUSED(errors)
    invalidateStatus();
}

void App::handleStateChanged(Qt::ApplicationState state)
{
    if (m_isGuiLoaded && ((state == Qt::ApplicationSuspended) || (state & Qt::ApplicationHidden))) {
        qDebug() << "App considered suspended/hidden, reducing polling, stopping UI processing";
        setCurrentControls(false);
        for (auto *const uiObject : m_uiObjects) {
            uiObject->moveToThread(nullptr);
        }
        if (m_unloadGuiWhenHidden || m_alwaysUnloadGuiWhenHidden) {
            m_unloadGuiWhenHidden = false;
            unloadMain();
        }
    } else if (state & Qt::ApplicationActive) {
        qDebug() << "App considered active, continuing polling, resuming UI processing";
        setCurrentControls(true);
        auto *const uiThread = thread();
        for (auto *const uiObject : m_uiObjects) {
            uiObject->moveToThread(uiThread);
        }
        if (!m_isGuiLoaded) {
            QtUtilities::deletePipelineCacheIfNeeded();
            m_engine ? loadMain() : initEngine();
        }
    }
}

void App::handleConnectionStatusChanged(Data::SyncthingStatus newStatus)
{
    invalidateStatus();
    Q_UNUSED(newStatus)
}

void App::handleLauncherStatusBroadcast(const QVariant &status)
{
    const auto launcherStatus = status.toMap();
    const auto isStarting = launcherStatus.value(QStringLiteral("isStarting"));
    const auto isStartingChanged = isStarting != m_launcherStatus.value(QStringLiteral("isStarting"));
    const auto isRunning = launcherStatus.value(QStringLiteral("isRunning"));
    const auto isRunningChanged = isRunning != m_launcherStatus.value(QStringLiteral("isRunning"));
    const auto guiUrl = launcherStatus.value(QStringLiteral("guiUrl"));
    const auto guiUrlChanged = guiUrl != m_launcherStatus.value(QStringLiteral("guiUrl"));
    m_launcherStatus = launcherStatus;
    if (isStartingChanged) {
        emit launchingChanged(isStarting.toBool());
    }
    if (isRunningChanged) {
        qDebug() << "Launcher running status has changed: " << isRunning;
        handleRunningChanged(isRunning.toBool());
    }
    if (guiUrlChanged) {
        qDebug() << "GUI address changed: " << guiUrl;
        handleGuiAddressChanged(guiUrl.toUrl());
    }
}

#ifdef Q_OS_ANDROID
static auto splitFolderRef(QStringView folderRef)
{
    struct {
        QStringView deviceId, folderId, folderLabel;
    } res;
    if (const auto separatorPos1 = folderRef.indexOf(QChar(':')); separatorPos1 >= 0) {
        res.deviceId = folderRef.mid(0, separatorPos1);
        res.folderId = folderRef.mid(separatorPos1 + 1);
        if (const auto separatorPos2 = res.folderId.indexOf(QChar(':')); separatorPos2 >= 0) {
            res.folderLabel = res.folderId.mid(separatorPos2 + 1);
            res.folderId = res.folderId.mid(0, separatorPos2);
        }
    } else {
        res.folderId = folderRef;
    }
    return res;
}

//QAndroidBinder *App::handleBind(const QAndroidIntent &intent)
//{
//    return nullptr;
//}

void App::handleAndroidIntent(const QString &data, bool fromNotification)
{
    qDebug() << "Handling Android intent: " << data;
    Q_UNUSED(fromNotification)
    if (data == QLatin1String("internalErrors")) {
        emit internalErrorsRequested();
    } else if (data == QLatin1String("connectionErrors")) {
        emit connectionErrorsRequested();
    } else if (data.startsWith(QLatin1String("sharedtext:"))) {
        emit textShared(data.mid(11));
    } else if (data.startsWith(QLatin1String("newdev:"))) {
        emit newDeviceTriggered(data.mid(7));
    } else if (data.startsWith(QLatin1String("newfolder:"))) {
        const auto folderRef = splitFolderRef(QStringView(data).mid(10));
        emit newDirTriggered(folderRef.deviceId.toString(), folderRef.folderId.toString(), folderRef.folderLabel.toString());
    }
}

void App::handleStoragePermissionChanged(bool storagePermissionGranted)
{
    if (!m_storagePermissionGranted.has_value() || m_storagePermissionGranted.value() != storagePermissionGranted) {
        emit storagePermissionGrantedChanged(m_storagePermissionGranted.emplace(storagePermissionGranted));
    }
}

void App::handleNotificationPermissionChanged(bool notificationPermissionGranted)
{
    if (!m_notificationPermissionGranted.has_value() || m_notificationPermissionGranted.value() != notificationPermissionGranted) {
        emit notificationPermissionGrantedChanged(m_notificationPermissionGranted.emplace(notificationPermissionGranted));
    }
}
#endif

void App::clearInternalErrors()
{
    if (m_internalErrors.isEmpty()) {
        return;
    }
#ifdef Q_OS_ANDROID
    // FIXME: tell service to clear notification
    //clearAndroidExtraNotifications(3, 3 + m_internalErrors.size());
#endif
    m_internalErrors.clear();
    invalidateStatus();
    emit hasInternalErrorsChanged();
}

bool App::postSyncthingConfig(const QJsonObject &rawConfig, const QJSValue &callback)
{
    if (m_pendingConfigChange.reply) {
        emit error(tr("Another config change is still pending."));
        return false;
    }
    m_pendingConfigChange = m_connection.postConfigFromJsonObject(rawConfig, [this, callback](QString &&error) {
        m_pendingConfigChange.reply = nullptr;
        emit savingConfigChanged(false);
        invalidateStatus();
        if (callback.isCallable()) {
            callback.call(QJSValueList{ error });
        }
    });
    connect(this, &QObject::destroyed, m_pendingConfigChange.reply, &QNetworkReply::deleteLater);
    connect(this, &QObject::destroyed, [c = m_pendingConfigChange.connection] { disconnect(c); });
    emit savingConfigChanged(true);
    invalidateStatus();
    return true;
}

bool App::invokeDirAction(const QString &dirId, const QString &action)
{
    if (action == QLatin1String("override")) {
        m_connection.requestOverride(dirId);
        return true;
    } else if (action == QLatin1String("revert")) {
        m_connection.requestRevert(dirId);
        return true;
    }
    return false;
}

bool QtGui::App::requestFromSyncthing(const QString &verb, const QString &path, const QVariantMap &parameters, const QJSValue &callback)
{
    auto params = QUrlQuery();
    for (const auto &parameter : parameters.asKeyValueRange()) {
        params.addQueryItem(parameter.first, parameter.second.toString());
    }
    auto query = m_connection.requestJsonData(verb.toUtf8(), path, params, QByteArray(), [this, callback](QJsonDocument &&doc, QString &&error) {
        if (callback.isCallable()) {
            callback.call(QJSValueList({ m_engine->toScriptValue(doc.object()), QJSValue(std::move(error)) }));
        }
    });
    connect(this, &QObject::destroyed, query.reply, &QNetworkReply::deleteLater);
    connect(this, &QObject::destroyed, [c = query.connection] { disconnect(c); });
    return true;
}

QString App::formatDataSize(quint64 size) const
{
    return QString::fromStdString(CppUtilities::dataSizeToString(size));
}

QString App::formatTraffic(quint64 total, double rate) const
{
    return trafficString(total, rate);
}

bool QtGui::App::hasDevice(const QString &id)
{
    auto row = 0;
    return m_connection.findDevInfo(id, row) != nullptr;
}

bool QtGui::App::hasDir(const QString &id)
{
    auto row = 0;
    return m_connection.findDirInfo(id, row) != nullptr;
}

QString App::deviceDisplayName(const QString &id) const
{
    auto row = 0;
    auto info = m_connection.findDevInfo(id, row);
    return info != nullptr ? info->displayName() : id;
}

QString App::dirDisplayName(const QString &id) const
{
    auto row = 0;
    auto info = m_connection.findDirInfo(id, row);
    return info != nullptr ? info->displayName() : id;
}

QVariantList App::computeDirsNeedingItems(const QModelIndex &devProxyModelIndex) const
{
    const auto *const devInfo = m_devModel.devInfo(m_sortFilterDevModel.mapToSource(devProxyModelIndex));
    auto dirs = QVariantList();
    if (!devInfo) {
        return dirs;
    }
    dirs.reserve(static_cast<QStringList::size_type>(devInfo->completionByDir.size()));
    for (const auto &[dirId, completion] : devInfo->completionByDir) {
        if (completion.needed.items < 1) {
            continue;
        }
        auto row = 0;
        auto dirNeedInfo = QVariantMap();
        auto *const dirInfo = m_connection.findDirInfo(dirId, row);
        dirNeedInfo.insert(QStringLiteral("dirId"), dirId);
        dirNeedInfo.insert(QStringLiteral("dirName"), dirInfo ? dirInfo->displayName() : dirId);
        dirNeedInfo.insert(QStringLiteral("items"), completion.needed.items);
        dirNeedInfo.insert(QStringLiteral("bytes"), completion.needed.bytes);
        dirs.emplace_back(std::move(dirNeedInfo));
    }
    return dirs;
}

bool App::minimize()
{
#ifdef Q_OS_ANDROID
    if (!QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jboolean>("minimize", "()Z")) {
        emit showError(tr("Unable to minimize app."));
        return false;
    }
    m_unloadGuiWhenHidden = true;
#else
    const auto windows = QGuiApplication::topLevelWindows();
    for (auto *const window : windows) {
        window->setWindowState(Qt::WindowMinimized);
    }
#endif
    return true;
}

void App::quit()
{
    m_app->quit();
}

void App::setPalette(const QColor &foreground, const QColor &background)
{
#ifdef SYNCTHING_APP_DARK_MODE_FROM_COLOR_SCHEME
    if (m_app) {
        auto palette = m_app->palette();
        palette.setColor(QPalette::Active, QPalette::Text, foreground);
        palette.setColor(QPalette::Active, QPalette::Base, background);
        palette.setColor(QPalette::Active, QPalette::WindowText, foreground);
        palette.setColor(QPalette::Active, QPalette::Window, background);
        m_app->setPalette(palette);
    }
#else
    Q_UNUSED(foreground)
    Q_UNUSED(background)
#endif
}

bool App::storeSettings()
{
    if (!m_settingsFile.isOpen() && !openSettings()) {
        return false;
    }
    if (!m_settingsFile.seek(0)) {
        return false;
    }
    const auto bytesWritten = m_settingsFile.write(QJsonDocument(m_settings).toJson(QJsonDocument::Compact));
    if (bytesWritten < m_settingsFile.size() && !m_settingsFile.resize(bytesWritten)) {
        return false;
    }
    if (m_settingsFile.error() != QFile::NoError) {
        emit error(tr("Unable to save settings: ") + m_settingsFile.errorString());
        return false;
    }
    m_serviceConnection.binder().transact(SyncthingServiceBinder::ReloadSettings, QAndroidParcel(), nullptr, QAndroidBinder::CallType::OneWay);
    return true;
}

bool App::applySettings()
{
    applySyncthingSettings();
    applyConnectionSettings(m_launcherStatus.value(QStringLiteral("guiUrl")).toUrl());
    auto tweaksSettings = m_settings.value(QLatin1String("tweaks")).toObject();
    m_alwaysUnloadGuiWhenHidden = tweaksSettings.value(QLatin1String("unloadGuiWhenHidden")).toBool(false);
    invalidateStatus();
    return true;
}

bool App::clearLogfile()
{
    auto launcherSettings = m_settings.value(QLatin1String("launcher"));
    auto launcherSettingsObj = launcherSettings.toObject();
    if (launcherSettingsObj.value(QLatin1String("writeLogFile")).toBool()) {
        launcherSettingsObj.insert(QLatin1String("writeLogFile"), false);
        m_settings.insert(QLatin1String("launcher"), launcherSettingsObj);
        // FIXME
        //applyLauncherSettings();
    }
    emit settingsChanged(m_settings);
    if (!storeSettings()) {
        return false;
    }

    /* FIXME: query logfile from launcher
    auto &logFile = m_launcher.logFile();
    if (logFile.fileName().isEmpty()) {
        logFile.setFileName(m_settingsDir->path() + QStringLiteral("/syncthing.log"));
    }
    auto ok = !logFile.exists() || logFile.remove();
    if (ok) {
        emit info(tr("Persistent logging disabled and logfile removed"));
    } else {
        emit info(tr("Unable to remove logfile"));
    }
    return ok;
    */
    return true;
}

bool App::checkOngoingImportExport()
{
    if (m_importExportStatus != ImportExportStatus::None) {
        emit info(tr("Another import/export still pending"));
        return true;
    } else {
        return false;
    }
}

void App::setImportExportStatus(ImportExportStatus importExportStatus)
{
    if (m_importExportStatus != importExportStatus) {
        m_importExportStatus = importExportStatus;
        emit importExportOngoingChanged(importExportStatus != ImportExportStatus::None);
        invalidateStatus();
    }
}

/*!
 * \brief Opens the Syncthing config file in the standard editor.
 */
bool QtGui::App::openSyncthingConfigFile()
{
    return openPath(m_syncthingConfigDir + QStringLiteral("/config.xml"));
}

/*!
 * \brief Checks the location specified via \a url for settings to import.
 */
bool App::checkSettings(const QUrl &url, const QJSValue &callback)
{
    if (checkOngoingImportExport()) {
        return false;
    }
    setImportExportStatus(ImportExportStatus::Checking);
    QtConcurrent::run([this, url, currentHomePath = currentSyncthingHomeDir()]() mutable {
        auto availableSettings = QVariantMap();
        auto pathStr = resolveUrl(url);
        auto path = std::filesystem::path(SYNCTHING_APP_STRING_CONVERSION(pathStr));
        auto errors = QStringList();
        availableSettings.insert(QStringLiteral("path"), pathStr);
        availableSettings.insert(QStringLiteral("currentSyncthingHomePath"), std::move(currentHomePath));
        try {
            auto appConfigPath = path / "appconfig.json";
            if (std::filesystem::exists(appConfigPath)) {
                availableSettings.insert(QStringLiteral("appConfigPath"), SYNCTHING_APP_PATH_CONVERSION(appConfigPath));
            }
            auto syncthingHomePath = path;
            auto syncthingHomePathNested = path / "syncthing";
            if (std::filesystem::is_directory(syncthingHomePathNested)) {
                syncthingHomePath = std::move(syncthingHomePathNested);
            }
            if (!std::filesystem::is_empty(syncthingHomePath)) {
                availableSettings.insert(QStringLiteral("syncthingHomePath"), SYNCTHING_APP_PATH_CONVERSION(syncthingHomePath));
            } else {
                errors.append(tr("The Syncthing home directory under \"%1\" is empty.").arg(SYNCTHING_APP_PATH_CONVERSION(syncthingHomePath)));
            }
            auto syncthingConfigPath = syncthingHomePath / "config.xml";
            if (std::filesystem::exists(syncthingConfigPath)) {
                auto syncthingConfigPathStr = SYNCTHING_APP_PATH_CONVERSION(syncthingConfigPath);
                auto syncthingConfig = Data::SyncthingConfig();
                syncthingConfig.restore(syncthingConfigPathStr, true);
                availableSettings.insert(QStringLiteral("syncthingConfigPath"), syncthingConfigPathStr);
                if (syncthingConfig.details.has_value()) {
                    availableSettings.insert(QStringLiteral("folders"), syncthingConfig.details->folders);
                    availableSettings.insert(QStringLiteral("devices"), syncthingConfig.details->devices);
                }
            } else {
                errors.append(tr("No Syncthing configuration file found under \"%1\".").arg(SYNCTHING_APP_PATH_CONVERSION(syncthingConfigPath)));
            }
        } catch (const std::filesystem::filesystem_error &e) {
            errors.append(QString::fromUtf8(e.what()));
        }
        if (!errors.isEmpty()) {
            availableSettings.insert(QStringLiteral("error"), errors.join(QChar('\n')));
        }
        return availableSettings;
    }).then(this, [this, callback](const QVariantMap &availableSettings) {
        setImportExportStatus(ImportExportStatus::None);
        if (callback.isCallable()) {
            callback.call(QJSValueList{ m_engine->toScriptValue(availableSettings) });
        }
    });
    return true;
}

/// \cond
static QJsonObject makeObjectFromDefaults(const QJsonObject &defaults, const QJsonObject &values)
{
    auto res = QJsonObject(defaults);
    const auto keys = values.keys();
    for (const auto &key : keys) {
        const auto value = values.value(key);
        if (!value.isUndefined() && !value.isNull()) {
            res.insert(key, value);
        }
    }
    res.insert(QStringLiteral("paused"), true);
    return res;
}

static QStringList::size_type importObjects(
    const QJsonObject &defaults, const QJsonArray &available, const QVariantList &selected, QJsonArray &destination)
{
    auto imported = QStringList::size_type();
    for (const auto &selectedIndex : selected) {
        auto ok = false;
        auto i = selectedIndex.toInt(&ok);
        if (ok && i >= 0 && i < available.size()) {
            destination.append(makeObjectFromDefaults(defaults, available.at(i).toObject()));
            ++imported;
        }
    }
    return imported;
}
/// \endcond

/*!
 * \brief Imports selected settings (app-related and of Syncthing itself) from the specified \a url.
 */
bool App::importSettings(const QVariantMap &availableSettings, const QVariantMap &selectedSettings, const QJSValue &callback)
{
    if (checkOngoingImportExport()) {
        return false;
    }
    if (!m_settingsDir.has_value()) {
        emit error(tr("Unable to import settings: settings directory was not located."));
        return false;
    }

    // stop Syncthing if necessary
    const auto importSyncthingHome = selectedSettings.value(QStringLiteral("syncthingHome")).toBool();
    if (importSyncthingHome && m_launcherStatus.value(QStringLiteral("isRunning")).toBool()) {
        emit info(tr("Waiting for backend to terminate before importing settings …"));
        m_settingsImport.first = availableSettings;
        m_settingsImport.second = selectedSettings;
        // FIXME
        //m_launcher.terminate();
        return false;
    }

    setImportExportStatus(ImportExportStatus::Importing);
    QtConcurrent::run([this, importSyncthingHome, availableSettings, selectedSettings, rawConfig = m_connection.rawConfig()]() mutable {
        // copy selected files from import directory to settings directory
        auto summary = QStringList();
        auto syncthingHomePath = availableSettings.value(QStringLiteral("currentSyncthingHomePath")).toString();
        try {
            // copy app config file
            const auto settingsPath = std::filesystem::path(SYNCTHING_APP_STRING_CONVERSION(m_settingsDir->path()));
            const auto importAppConfig = selectedSettings.value(QStringLiteral("appConfig")).toBool();
            if (importAppConfig) {
                const auto appConfigSrcPathStr = availableSettings.value(QStringLiteral("appConfigPath")).toString();
                const auto appConfigSrcPath = SYNCTHING_APP_STRING_CONVERSION(appConfigSrcPathStr);
                const auto appConfigDstPath = settingsPath / "appconfig.json";
                auto appConfigSrcFile = QFile();
                auto appConfigObj = QJsonObject();
                if (const auto errorMessage = openSettingFile(appConfigSrcFile, appConfigSrcPathStr); !errorMessage.isEmpty()) {
                    throw std::runtime_error(errorMessage.toStdString());
                }
                if (const auto errorMessage = readSettingFile(appConfigSrcFile, appConfigObj); !errorMessage.isEmpty()) {
                    throw std::runtime_error(errorMessage.toStdString());
                }
                const auto newLauncherSettings = appConfigObj.value(QLatin1String("launcher")).toObject();
                if (newLauncherSettings.contains(QLatin1String("stHomePath"))) {
                    syncthingHomePath = newLauncherSettings.value(QLatin1String("stHomePath")).toString();
                }
                appConfigSrcFile.close();
                std::filesystem::copy_file(appConfigSrcPath, appConfigDstPath, std::filesystem::copy_options::overwrite_existing);
                summary.append(tr("Imported app config from \"%1\".").arg(appConfigSrcPathStr));
            }

            // copy Syncthing home directory
            if (importSyncthingHome) {
                const auto homeSrcPathStr = availableSettings.value(QStringLiteral("syncthingHomePath")).toString();
                const auto homeSrcPath = SYNCTHING_APP_STRING_CONVERSION(homeSrcPathStr);
                const auto homeDstPath = syncthingHomePath.isEmpty() ? (settingsPath / "syncthing")
                                                                     : std::filesystem::path(SYNCTHING_APP_STRING_CONVERSION(syncthingHomePath));
                std::filesystem::remove_all(homeDstPath);
                std::filesystem::create_directory(homeDstPath);
                std::filesystem::copy(
                    homeSrcPath, homeDstPath, std::filesystem::copy_options::update_existing | std::filesystem::copy_options::recursive);
                summary.append(tr("Imported Syncthing config and database from \"%1\".").arg(homeSrcPathStr));
            }
        } catch (const std::runtime_error &e) {
            return std::make_pair(QString::fromUtf8(e.what()), true);
        }

        // merge selected folders/devices into existing config
        if (!importSyncthingHome) {
            const auto availableFolders = availableSettings.value(QStringLiteral("folders")).toJsonArray();
            const auto availableDevices = availableSettings.value(QStringLiteral("devices")).toJsonArray();
            const auto selectedFolders = selectedSettings.value(QStringLiteral("selectedFolders")).value<QVariantList>();
            const auto selectedDevices = selectedSettings.value(QStringLiteral("selectedDevices")).value<QVariantList>();
            const auto defaults = rawConfig.value(QStringLiteral("defaults")).toObject();
            const auto folderTemplate = defaults.value(QStringLiteral("folder")).toObject();
            const auto deviceTemplate = defaults.value(QStringLiteral("device")).toObject();
            const auto foldersVal = rawConfig.value(QStringLiteral("folders"));
            const auto devicesVal = rawConfig.value(QStringLiteral("devices"));
            if (!foldersVal.isArray() || !devicesVal.isArray()) {
                return std::make_pair(tr("Unable to find folders/devices in current Syncthing config."), true);
            }
            auto folders = foldersVal.toArray();
            auto devices = devicesVal.toArray();
            const auto importedFolders = importObjects(folderTemplate, availableFolders, selectedFolders, folders);
            const auto importedDevices = importObjects(deviceTemplate, availableDevices, selectedDevices, devices);
            if (importedFolders || importedDevices) {
                if (importedFolders) {
                    rawConfig.insert(QStringLiteral("folders"), folders);
                }
                if (importedDevices) {
                    rawConfig.insert(QStringLiteral("devices"), devices);
                }
                auto newConfigJson = QJsonDocument(rawConfig).toJson(QJsonDocument::Compact);
                if (QMetaObject::invokeMethod(&m_connection, &SyncthingConnection::postRawConfig, Qt::QueuedConnection, std::move(newConfigJson))) {
                    summary.append(tr("Merging %1 folders and %2 devices").arg(importedFolders).arg(importedDevices));
                } else {
                    return std::make_pair(tr("Unable to import folders/devices."), true);
                }
            }
        }

        if (summary.isEmpty()) {
            summary.append(tr("Nothing has been imported."));
        }
        return std::make_pair(summary.join(QChar('\n')), false);
    }).then(this, [this, callback](const std::pair<QString, bool> &res) {
        setImportExportStatus(ImportExportStatus::None);
        m_settingsImport.first.clear();
        m_settingsImport.second.clear();

        if (callback.isCallable()) {
            callback.call(QJSValueList{ QJSValue(res.first), QJSValue(res.second) });
        }
        if (res.second) {
            emit error(tr("Unable to import settings: %1").arg(res.first));
        } else {
            emit info(res.first);
        }

        // reload settings
        m_settingsFile.close();
        loadSettings();
        applySettings();
    });
    return true;
}

/*!
 * \brief Exports all settings (app-related and of Syncthing itself) to the specified \a url.
 */
bool App::exportSettings(const QUrl &url, const QJSValue &callback)
{
    if (checkOngoingImportExport()) {
        return false;
    }
    setImportExportStatus(ImportExportStatus::Exporting);

    QtConcurrent::run([this, url, currentHomePath = currentSyncthingHomeDir()] {
        const auto path = resolveUrl(url);
        const auto dir = QDir(path);
        if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
            return std::make_pair(tr("unable to create export directory under \"%1\"").arg(path), true);
        }
        if (!m_settingsDir.has_value()) {
            return std::make_pair(tr("settings directory was not located."), true);
        }
        try {
            std::filesystem::copy(SYNCTHING_APP_STRING_CONVERSION(m_settingsDir->path()), SYNCTHING_APP_STRING_CONVERSION(path),
                std::filesystem::copy_options::update_existing | std::filesystem::copy_options::recursive);
            if (!currentHomePath.isEmpty()) {
                std::filesystem::copy(SYNCTHING_APP_STRING_CONVERSION(currentHomePath),
                    SYNCTHING_APP_STRING_CONVERSION(path + QStringLiteral("/syncthing")),
                    std::filesystem::copy_options::update_existing | std::filesystem::copy_options::recursive);
            }
        } catch (const std::filesystem::filesystem_error &e) {
            return std::make_pair(QString::fromUtf8(e.what()), true);
        }
        return std::make_pair(tr("Settings have been exported to \"%1\".").arg(path), false);
    }).then(this, [this, callback](const std::pair<QString, bool> &res) {
        setImportExportStatus(ImportExportStatus::None);
        if (callback.isCallable()) {
            callback.call(QJSValueList{ QJSValue(res.first), QJSValue(res.second) });
        }
        if (res.second) {
            emit error(tr("Unable to export settings: %1").arg(res.first));
        } else {
            emit info(res.first);
        }
    });
    return true;
}

/*!
 * \brief Returns whether \a path is populated if it points to a directory usable as Syncthing home.
 */
QVariant App::isPopulated(const QString &path) const
{
    auto ec = std::error_code();
    const auto stdPath = std::filesystem::path(SYNCTHING_APP_STRING_CONVERSION(path));
    const auto status = std::filesystem::symlink_status(stdPath, ec);
    const auto existing = std::filesystem::exists(status);
    return (existing && !std::filesystem::is_directory(status)) ? QVariant() : QVariant(existing && !std::filesystem::is_empty(stdPath, ec));
}

QString App::currentSyncthingHomeDir() const
{
    return m_settings.value(QLatin1String("launcher")).toObject().value(QLatin1String("stHomeDir")).toString();
}

bool App::checkSyncthingHome(const QJSValue &callback)
{
    if (checkOngoingImportExport()) {
        return false;
    }
    setImportExportStatus(ImportExportStatus::CheckingMove);
    QtConcurrent::run([this, defaultPath = m_settingsDir.value_or(QDir()).path(), currentVal = currentSyncthingHomeDir()]() mutable {
        const auto externalDirs = externalStoragePaths();
        auto availableDirs = QVariantList();
        auto defaultDir = QVariantMap();
        defaultDir[QStringLiteral("path")] = (defaultPath += QStringLiteral("/syncthing"));
        defaultDir[QStringLiteral("value")] = QString();
        defaultDir[QStringLiteral("label")] = tr("Default directory");
        defaultDir[QStringLiteral("selected")] = currentVal.isEmpty();
        defaultDir[QStringLiteral("populated")] = isPopulated(defaultPath);
        availableDirs.reserve(externalDirs.size() + 1);
        availableDirs.emplace_back(defaultDir);
        auto externalStorageNumber = 0;
        auto hasCurrentPath = false;
        for (const auto &externalDir : externalDirs) {
            const auto path = externalDir + QStringLiteral("/syncthing");
            auto dir = QVariantMap();
            auto isCurrentPath = currentVal == path;
            if (isCurrentPath) {
                hasCurrentPath = true;
            }
            dir[QStringLiteral("path")] = path;
            dir[QStringLiteral("value")] = path;
            dir[QStringLiteral("label")] = tr("External storage %1").arg(++externalStorageNumber);
            dir[QStringLiteral("selected")] = isCurrentPath;
            dir[QStringLiteral("populated")] = isPopulated(path);
            availableDirs.emplace_back(dir);
        }
        if (!currentVal.isEmpty() && !hasCurrentPath) {
            auto dir = QVariantMap();
            dir[QStringLiteral("path")] = currentVal;
            dir[QStringLiteral("value")] = currentVal;
            dir[QStringLiteral("label")] = tr("Current home directory");
            dir[QStringLiteral("selected")] = true;
            dir[QStringLiteral("populated")] = isPopulated(currentVal);
            availableDirs.emplace_back(dir);
        }
        auto currentHome = currentVal.isEmpty() ? defaultPath : currentVal;
        auto homeDirInfo = QVariantMap();
        homeDirInfo[QStringLiteral("currentHome")] = currentHome;
        homeDirInfo[QStringLiteral("currentHomePopulated")] = isPopulated(currentHome);
        homeDirInfo[QStringLiteral("availableDirs")] = std::move(availableDirs);
        return homeDirInfo;
    }).then(this, [this, callback](const QVariantMap &homeDirInfo) {
        setImportExportStatus(ImportExportStatus::None);
        if (callback.isCallable()) {
            callback.call(QJSValueList{ m_engine->toScriptValue(homeDirInfo) });
        }
    });
    return true;
}

bool App::moveSyncthingHome(const QString &newHomeDir, const QJSValue &callback)
{
    if (checkOngoingImportExport()) {
        return false;
    }
    if (!m_settingsDir.has_value()) {
        emit error(tr("Unable to import settings: settings directory was not located."));
        return false;
    }

    // stop Syncthing if necessary
    /* FIXME: query service
    if (m_launcher.isRunning()) {
        emit info(tr("Waiting for backend to terminate before moving home …"));
        m_homeDirMove = newHomeDir;
        m_launcher.terminate();
        return false;
    }
    */

    setImportExportStatus(ImportExportStatus::Moving);
    QtConcurrent::run([newHomeDir = newHomeDir, customHomeDir = currentSyncthingHomeDir(), defaultPath = m_settingsDir.value_or(QDir()).path(),
                          rawConfig = m_connection.rawConfig()]() mutable {
        // determine paths
        const auto sourceDir = customHomeDir.isEmpty() ? defaultPath + QStringLiteral("/syncthing") : customHomeDir;
        const auto sourceDirStd = std::filesystem::path(SYNCTHING_APP_STRING_CONVERSION(sourceDir));
        const auto destinationDir = newHomeDir.isEmpty() ? defaultPath + QStringLiteral("/syncthing") : newHomeDir;
        const auto destinationDirStd = std::filesystem::path(SYNCTHING_APP_STRING_CONVERSION(destinationDir));

        // return early if source and destination are equivalent
        if (newHomeDir.isNull()) {
            newHomeDir = QLatin1String("");
        }
        if (auto ec = std::error_code(); std::filesystem::equivalent(sourceDirStd, destinationDirStd, ec) && !ec) {
            return std::make_tuple(newHomeDir, tr("Home directory stays the same."), false);
        }

        // copy files from source directory (if existing) to destination directory
        auto summary = QStringList();
        auto copied = false;
        try {
            const auto sourceStatus = std::filesystem::symlink_status(sourceDirStd);
            const auto destinationStatus = std::filesystem::symlink_status(destinationDirStd);
            if (std::filesystem::is_directory(sourceStatus) && !std::filesystem::is_empty(sourceDirStd)) {
                std::filesystem::remove_all(destinationDirStd);
                summary.append(tr("Cleaned up new home directory \"%1\".").arg(destinationDir));

                std::filesystem::create_directory(destinationDirStd);
                std::filesystem::copy(
                    sourceDirStd, destinationDirStd, std::filesystem::copy_options::update_existing | std::filesystem::copy_options::recursive);
                copied = true;
                summary.append(tr("Copied data from previous home directory \"%1\" to new one.").arg(sourceDir));

                std::filesystem::remove_all(sourceDirStd);
                summary.append(tr("Cleaned up previous home directory."));
            } else if (std::filesystem::is_directory(destinationStatus)) {
                if (std::filesystem::is_empty(destinationDirStd)) {
                    summary.append(tr("Configured \"%1\" as new/empty Syncthing home.").arg(destinationDir));
                } else {
                    summary.append(tr("Configured \"%1\" as Syncthing home.").arg(destinationDir));
                }
            }
        } catch (const std::filesystem::filesystem_error &e) {
            summary.append(tr("Unable to move home directory: %1").arg(QString::fromUtf8(e.what())));
            return std::make_tuple(copied ? newHomeDir : QString(), summary.join(QChar('\n')), true);
        }
        return std::make_tuple(newHomeDir, summary.join(QChar('\n')), false);
    }).then(this, [this, callback](const std::tuple<QString, QString, bool> &res) {
        m_homeDirMove.reset();

        auto [setHomeDir, message, errorOccurred] = res;
        if (!setHomeDir.isNull()) {
            auto launcherSettings = m_settings.value(QLatin1String("launcher")).toObject();
            launcherSettings.insert(QLatin1String("stHomeDir"), setHomeDir);
            m_settings.insert(QLatin1String("launcher"), launcherSettings);
            storeSettings();
            emit settingsChanged(m_settings);
        }

        setImportExportStatus(ImportExportStatus::None);
        if (callback.isCallable()) {
            callback.call(QJSValueList{ QJSValue(message), QJSValue(errorOccurred) });
        }
        emit errorOccurred ? error(message) : info(message);
        applySettings();
    });
    return true;
}

void App::applyDarkmodeChange(bool isDarkColorSchemeEnabled, bool isDarkPaletteEnabled)
{
    m_darkColorScheme = isDarkColorSchemeEnabled;
    m_darkPalette = isDarkPaletteEnabled;
    const auto isDarkmodeEnabled = m_darkColorScheme || m_darkPalette;
    m_qtSettings.reapplyDefaultIconTheme(isDarkmodeEnabled);
    if (isDarkmodeEnabled == m_darkmodeEnabled) {
        return;
    }
    qDebug() << "Darkmode has changed: " << isDarkmodeEnabled;
    m_darkmodeEnabled = isDarkmodeEnabled;
    m_dirModel.setBrightColors(isDarkmodeEnabled);
    m_devModel.setBrightColors(isDarkmodeEnabled);
    m_changesModel.setBrightColors(isDarkmodeEnabled);
    emit darkmodeEnabledChanged(isDarkmodeEnabled);
}

} // namespace QtGui
