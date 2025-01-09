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

#include <qtutilities/misc/desktoputils.h>

#include <qtforkawesome/renderer.h>
#include <qtquickforkawesome/imageprovider.h>

#include <c++utilities/conversion/stringconversion.h>

#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonValue>
#include <QNetworkReply>
#include <QQmlContext>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QStringBuilder>
#include <QUrlQuery>
#include <QtConcurrent/QtConcurrentRun>

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
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

#ifdef Q_OS_WINDOWS
#define SYNCTHING_APP_STRING_CONVERSION(s) s.toStdWString()
#define SYNCTHING_APP_PATH_CONVERSION(s) QString::fromStdWString(s.wstring())
#else
#define SYNCTHING_APP_STRING_CONVERSION(s) s.toLocal8Bit().toStdString()
#define SYNCTHING_APP_PATH_CONVERSION(s) QString::fromLocal8Bit(s.string())
#endif

using namespace Data;

namespace QtGui {

static void deletePipelineCache()
{
#ifdef Q_OS_ANDROID
    // delete OpenGL pipeline cache under Android as it seems to break loading the app in certain cases
    const auto cachePaths = QStandardPaths::standardLocations(QStandardPaths::CacheLocation);
    for (const auto &cachePath : cachePaths) {
        const auto cacheDir = QDir(cachePath);
        const auto subdirs = cacheDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const auto &subdir : subdirs) {
            if (subdir.startsWith(QLatin1String("qtpipelinecache"))) {
                QFile::remove(cachePath % QChar('/') % subdir % QStringLiteral("/qqpc_opengl"));
            }
        }
    }
#endif
}

#ifdef Q_OS_ANDROID
// define functions called from Java
static App *appObjectForJava = nullptr;

static void onAndroidIntent(JNIEnv *, jobject, jstring page, jboolean fromNotification)
{
    QMetaObject::invokeMethod(appObjectForJava, "handleAndroidIntent", Qt::QueuedConnection,
        Q_ARG(QString, QJniObject::fromLocalRef(page).toString()), Q_ARG(bool, fromNotification));
}
#endif

App::App(bool insecure, QObject *parent)
    : QObject(parent)
    , m_app(static_cast<QGuiApplication *>(QCoreApplication::instance()))
    , m_notifier(m_connection)
    , m_dirModel(m_connection)
    , m_sortFilterDirModel(&m_dirModel)
    , m_devModel(m_connection)
    , m_sortFilterDevModel(&m_devModel)
    , m_changesModel(m_connection)
    , m_faUrlBase(QStringLiteral("image://fa/"))
    , m_uiObjects({&m_dirModel, &m_sortFilterDirModel, &m_devModel, &m_sortFilterDevModel, &m_changesModel})
    , m_iconSize(16)
    , m_tabIndex(-1)
    , m_importExportStatus(ImportExportStatus::None)
    , m_insecure(insecure)
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    , m_connectToLaunched(true)
#else
    , m_connectToLaunched(false)
#endif
    , m_darkmodeEnabled(false)
    , m_darkColorScheme(false)
    , m_darkPalette(QtUtilities::isPaletteDark())
    , m_isGuiLoaded(false)
    , m_alwaysUnloadGuiWhenHidden(false)
    , m_unloadGuiWhenHidden(false)
{
    m_app->installEventFilter(this);
    m_app->setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
    connect(m_app, &QGuiApplication::applicationStateChanged, this, &App::handleStateChanged);

    deletePipelineCache();
    loadSettings();
    applySettings();
    QtUtilities::onDarkModeChanged([this](bool darkColorScheme) { applyDarkmodeChange(darkColorScheme, m_darkPalette); }, this);

    auto &iconManager = Data::IconManager::instance();
    auto statusIconSettings = StatusIconSettings();
    statusIconSettings.strokeWidth = StatusIconStrokeWidth::Thick;
    iconManager.applySettings(&statusIconSettings, &statusIconSettings, true, true);
    connect(&iconManager, &Data::IconManager::statusIconsChanged, this, &App::statusInfoChanged);
#ifdef Q_OS_ANDROID
    connect(&iconManager, &Data::IconManager::statusIconsChanged, this, &App::invalidateAndroidIconCache);
#endif

    m_sortFilterDirModel.sort(0, Qt::AscendingOrder);
    m_sortFilterDevModel.sort(0, Qt::AscendingOrder);

    m_connection.setPollingFlags(SyncthingConnection::PollingFlags::MainEvents | SyncthingConnection::PollingFlags::Errors);
#ifdef Q_OS_ANDROID
    m_notifier.setEnabledNotifications(SyncthingHighLevelNotification::ConnectedDisconnected | SyncthingHighLevelNotification::NewDevice | SyncthingHighLevelNotification::NewDir);
#else
    m_notifier.setEnabledNotifications(SyncthingHighLevelNotification::ConnectedDisconnected);
#endif
    connect(&m_connection, &SyncthingConnection::error, this, &App::handleConnectionError);
    connect(&m_connection, &SyncthingConnection::statusChanged, this, &App::invalidateStatus);
    connect(&m_connection, &SyncthingConnection::newDevices, this, &App::handleChangedDevices);
    connect(&m_connection, &SyncthingConnection::autoReconnectIntervalChanged, this, &App::invalidateStatus);
    connect(&m_connection, &SyncthingConnection::hasOutOfSyncDirsChanged, this, &App::invalidateStatus);
    connect(&m_connection, &SyncthingConnection::devStatusChanged, this, &App::handleChangedDevices);
    connect(&m_connection, &SyncthingConnection::newErrors, this, &App::handleNewErrors);
#ifdef Q_OS_ANDROID
    connect(&m_connection, &SyncthingConnection::newNotification, this, &App::updateSyncthingErrorsNotification);
    connect(&m_notifier, &SyncthingNotifier::newDevice, this, &App::showNewDevice);
    connect(&m_notifier, &SyncthingNotifier::newDir, this, &App::showNewDir);
#endif

    m_launcher.setEmittingOutput(true);
    connect(&m_launcher, &SyncthingLauncher::outputAvailable, this, &App::gatherLogs);
    connect(&m_launcher, &SyncthingLauncher::runningChanged, this, &App::handleRunningChanged);
    connect(&m_launcher, &SyncthingLauncher::guiUrlChanged, this, &App::handleGuiAddressChanged);

    connect(
        &m_engine, &QQmlApplicationEngine::objectCreated, m_app,
        [](QObject *obj, const QUrl &objUrl) {
            if (!obj) {
                std::cerr << "Unable to load " << objUrl.toString().toStdString() << '\n';
                QCoreApplication::exit(EXIT_FAILURE);
            }
        },
        Qt::QueuedConnection);
    connect(&m_engine, &QQmlApplicationEngine::quit, m_app, &QGuiApplication::quit);
    m_engine.setProperty("app", QVariant::fromValue(this));
    m_engine.addImageProvider(QStringLiteral("fa"), new QtForkAwesome::QuickImageProvider(QtForkAwesome::Renderer::global()));

#ifdef Q_OS_ANDROID
    // register native methods of Android activity
    if (!appObjectForJava) {
        appObjectForJava = this;
        auto env = QJniEnvironment();
        auto registeredMethods = true;
        static const JNINativeMethod activityMethods[] = {
            { "onAndroidIntent", "(Ljava/lang/String;Z)V", reinterpret_cast<void *>(onAndroidIntent) },
        };
        registeredMethods = env.registerNativeMethods("io/github/martchus/syncthingtray/Activity", activityMethods, 1) && registeredMethods;
        if (!registeredMethods) {
            qWarning() << "Unable to register all native methods in JNI environment.";
        }
    }

    // start Android service
    QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("startSyncthingService");
#endif

    loadMain();

    qDebug() << "App initialized";
}

App *App::create(QQmlEngine *, QJSEngine *engine)
{
    auto *const app = engine->property("app").value<App *>();
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
    }
    if (m_connectToLaunched) {
        if (!m_launcher.isRunning()) {
            return m_status.emplace(m_launcher.runningStatus());
        } else if (m_launcher.isStarting()) {
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

QVariantMap App::statistics() const
{
    auto stats = QVariantMap();
    statistics(stats);
    return stats;
}

void App::statistics(QVariantMap &res) const
{
    auto dbDir = QDir(m_syncthingDataDir + QStringLiteral("/index-v0.14.0.db"));
    auto dbFiles = dbDir.entryInfoList({ QStringLiteral("*.ldb") }, QDir::Files);
    auto dbSize = std::accumulate(dbFiles.begin(), dbFiles.end(), qint64(), [](auto size, const auto &dbFile) { return size + dbFile.size(); });
    res[QStringLiteral("stConfigDir")] = m_syncthingConfigDir;
    res[QStringLiteral("stDataDir")] = m_syncthingDataDir;
    res[QStringLiteral("stDbSize")] = QString::fromStdString(CppUtilities::dataSizeToString(static_cast<std::uint64_t>(dbSize)));
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

bool App::loadMain()
{
    // allow overriding Qml entry point for hot-reloading; otherwise load proper Qml module from resources
    qDebug() << "Loading Qt Quick GUI";
    if (const auto path = qEnvironmentVariable(PROJECT_VARNAME_UPPER "_QML_MAIN_PATH"); !path.isEmpty()) {
        qDebug() << "Path Qml entry point for Qt Quick GUI was overriden to: " << path;
        m_engine.load(path);
    } else {
        m_engine.loadFromModule("Main", "Main");
    }
    m_isGuiLoaded = true;
    return true;
}

bool App::reloadMain()
{
    qDebug() << "Reloading Qt Quick GUI";
    unloadMain();
    m_engine.clearComponentCache();
    return loadMain();
}

bool App::unloadMain()
{
    qDebug() << "Unloading Qt Quick GUI";
    const auto rootObjects = m_engine.rootObjects();
    for (auto *const rootObject : rootObjects) {
        rootObject->deleteLater();
    }
    m_isGuiLoaded = false;
    return true;
}

void App::shutdown()
{
    m_launcher.stopLibSyncthing();
#ifdef Q_OS_ANDROID
    QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("stopSyncthingService");
#endif
}

bool App::openPath(const QString &path)
{
#ifdef Q_OS_ANDROID
    if (QJniObject(QNativeInterface::QAndroidApplication::context())
            .callMethod<jboolean>("openPath", "(Ljava/lang/String;)Z", QJniObject::fromString(path))) {
#else
    if (QtUtilities::openLocalFileOrDir(path)) {
#endif
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
        auto array = m_engine.newArray(static_cast<quint32>(dir.itemErrors.size()));
        auto index = quint32();
        for (const auto &itemError : dir.itemErrors) {
            auto error = m_engine.newObject();
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
    auto query = m_connection.requestJsonData(QByteArrayLiteral("GET"), QStringLiteral("svc/report"), QUrlQuery(), QByteArray(), [this, callback](QJsonDocument &&doc, QString &&error) {
        auto report = doc.object().toVariantMap();
        statistics(report);
        callback.call(QJSValueList({ m_engine.toScriptValue(report), QJSValue(std::move(error)) }));
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

bool App::eventFilter(QObject *object, QEvent *event)
{
    if (object != m_app) {
        return false;
    }
    switch (event->type()) {
    case QEvent::ApplicationPaletteChange:
        qDebug() << "Application palette has changed";
        applyDarkmodeChange(m_darkColorScheme, QtUtilities::isPaletteDark());
        break;
    default:;
    }
    return false;
}

void App::handleConnectionError(
    const QString &errorMessage, Data::SyncthingErrorCategory category, int networkError, const QNetworkRequest &request, const QByteArray &response)
{
    if (!InternalError::isRelevant(m_connection, category, errorMessage, networkError)) {
        return;
    }
    qWarning() << "Connection error: " << errorMessage;
    auto error = InternalError(errorMessage, request.url(), response);
    emit internalError(error);
#ifdef Q_OS_ANDROID
    showInternalError(error);
#endif
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
    static const auto problematicFileSystems = QSet<QByteArray>({
        QByteArrayLiteral("fat"), QByteArrayLiteral("vfat"), QByteArrayLiteral("exfat")
    });
    const auto storageInfo = QStorageInfo(path);
    return storageInfo.isValid() && problematicFileSystems.contains(storageInfo.fileSystemType());
#endif
}

void App::invalidateStatus()
{
    m_status.reset();
    m_statusInfo.updateConnectionStatus(m_connection);
    m_statusInfo.updateConnectedDevices(m_connection);
    emit statusInfoChanged();
#ifdef Q_OS_ANDROID
    updateAndroidNotification();
#endif
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
    }
}

void App::handleGuiAddressChanged(const QUrl &newUrl)
{
    m_connectionSettingsFromLauncher.syncthingUrl = newUrl.toString();
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

void App::handleChangedDevices()
{
    m_statusInfo.updateConnectedDevices(m_connection);
    emit statusInfoChanged();
#ifdef Q_OS_ANDROID
    updateAndroidNotification();
#endif
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
            deletePipelineCache();
            loadMain();
        }
    }
}

#ifdef Q_OS_ANDROID
void App::invalidateAndroidIconCache()
{
    m_statusInfo.updateConnectionStatus(m_connection);
    m_androidIconCache.clear();
    updateAndroidNotification();
}

QJniObject &App::makeAndroidIcon(const QIcon &icon)
{
    auto &cachedIcon = m_androidIconCache[&icon];
    if (!cachedIcon.isValid()) {
        cachedIcon = IconManager::makeAndroidBitmap(icon.pixmap(QSize(32, 32)).toImage());
    }
    return cachedIcon;
}

void App::updateAndroidNotification()
{
    const auto title = QJniObject::fromString(m_connection.isConnected() ? m_statusInfo.statusText() : status());
    const auto text = QJniObject::fromString(m_statusInfo.additionalStatusText());
    static const auto subText = QJniObject::fromString(QString());
    const auto &icon = makeAndroidIcon(m_statusInfo.statusIcon());
    QJniObject::callStaticMethod<void>("io/github/martchus/syncthingtray/SyncthingService", "updateNotification",
        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Landroid/graphics/Bitmap;)V", title.object(), text.object(), subText.object(),
        icon.object());
}

void App::updateExtraAndroidNotification(
    const QJniObject &title, const QJniObject &text, const QJniObject &subText, const QJniObject &page, const QJniObject &icon, int id)
{
    QJniObject::callStaticMethod<void>("io/github/martchus/syncthingtray/SyncthingService", "updateExtraNotification",
        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Landroid/graphics/Bitmap;I)V", title.object(), text.object(),
        subText.object(), page.object(), icon.object(), id ? id : ++m_androidNotificationId);
}

void App::clearAndroidExtraNotifications(int firstId, int lastId)
{
    QJniObject::callStaticMethod<void>("io/github/martchus/syncthingtray/SyncthingService", "cancelExtraNotification", "(II)V", firstId, lastId);
}

void App::updateSyncthingErrorsNotification(CppUtilities::DateTime when, const QString &message)
{
    auto whenString = when.toString();
    m_syncthingErrors.reserve(m_syncthingErrors.size() + 1 + whenString.size() + 2 + message.size());
    if (!m_syncthingErrors.isEmpty()) {
        m_syncthingErrors += QChar('\n');
    }
    m_syncthingErrors += std::move(whenString);
    m_syncthingErrors += QChar(':');
    m_syncthingErrors += QChar(' ');
    m_syncthingErrors += message;

    const auto title = QJniObject::fromString(tr("Syncthing errors/notifications"));
    static const auto text = QJniObject::fromString(QString());
    const auto subText = QJniObject::fromString(m_syncthingErrors);
    static const auto page = QJniObject::fromString(QStringLiteral("connectionErrors"));
    const auto &icon = makeAndroidIcon(commonForkAwesomeIcons().exclamation);
    updateExtraAndroidNotification(title, text, subText, page, icon, 2);
}

void App::clearSyncthingErrorsNotification()
{
    m_syncthingErrors.clear();
    clearAndroidExtraNotifications(2, -1);
}

void App::showInternalError(const InternalError &error)
{
    const auto title = QJniObject::fromString(m_internalErrors.empty()
        ? tr("Syncthing API error")
        : tr("%1 Syncthing API errors").arg(m_internalErrors.size() + 1));
    const auto text = QJniObject::fromString(m_internalErrors.empty()
        ? error.message
        : tr("most recent: ") + error.message);
    const auto subText = QJniObject::fromString(error.url.isEmpty() ? QString() : QStringLiteral("URL: ") + error.url.toString());
    static const auto page = QJniObject::fromString(QStringLiteral("internalErrors"));
    const auto &icon = makeAndroidIcon(commonForkAwesomeIcons().exclamation);
    updateExtraAndroidNotification(title, text, subText, page, icon, 3);
}

void App::showNewDevice(const QString &devId, const QString &message)
{
    const auto title = QJniObject::fromString(tr("Syncthing device wants to connect"));
    const auto text = QJniObject::fromString(message);
    static const auto subText = QJniObject::fromString(QString());
    const auto page = QJniObject::fromString(QStringLiteral("newdev:") + devId);
    const auto &icon = makeAndroidIcon(commonForkAwesomeIcons().networkWired);
    updateExtraAndroidNotification(title, text, subText, page, icon);
}

void App::showNewDir(const QString &devId, const QString &dirId, const QString &dirLabel, const QString &message)
{
    const auto title = QJniObject::fromString(tr("Syncthing device wants to share folder"));
    const auto text = QJniObject::fromString(message);
    static const auto subText = QJniObject::fromString(QString());
    const auto page = QJniObject::fromString(QStringLiteral("newfolder:") % devId % QChar(':') % dirId % QChar(':') % dirLabel);
    const auto &icon = makeAndroidIcon(commonForkAwesomeIcons().shareAlt);
    updateExtraAndroidNotification(title, text, subText, page, icon);
}

static auto splitFolderRef(QStringView folderRef)
{
    struct { QStringView deviceId, folderId, folderLabel; } res;
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
#endif

void App::clearInternalErrors()
{
    if (m_internalErrors.isEmpty()) {
        return;
    }
#ifdef Q_OS_ANDROID
    clearAndroidExtraNotifications(3, 3 + m_internalErrors.size());
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

bool QtGui::App::requestFromSyncthing(const QString &verb, const QString &path, const QVariantMap &parameters, const QJSValue &callback)
{
    auto params = QUrlQuery();
    for (const auto &parameter : parameters.asKeyValueRange()) {
        params.addQueryItem(parameter.first, parameter.second.toString());
    }
    auto query = m_connection.requestJsonData(verb.toUtf8(), path, params, QByteArray(), [this, callback](QJsonDocument &&doc, QString &&error) {
        if (callback.isCallable()) {
            callback.call(QJSValueList({ m_engine.toScriptValue(doc.object()), QJSValue(std::move(error)) }));
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
    return QString::fromStdString(CppUtilities::bitrateToString(rate, true)) % QChar(',') % QChar(' ') % QChar('(')
        % QString::fromStdString(CppUtilities::dataSizeToString(total)) % QChar(')');
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

bool App::openSettings()
{
    if (!m_settingsDir.has_value()) {
        m_settingsDir.emplace(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    }
    if (!m_settingsDir->exists() && !m_settingsDir->mkpath(QStringLiteral("."))) {
        emit error(tr("Unable to create settings directory under \"%1\".").arg(m_settingsDir->path()));
        m_settingsDir.reset();
        return false;
    }
    m_settingsFile.close();
    m_settingsFile.unsetError();
    m_settingsFile.setFileName(m_settingsDir->path() + QStringLiteral("/appconfig.json"));
    if (!m_settingsFile.open(QFile::ReadWrite)) {
        m_settingsDir.reset();
        emit error(tr("Unable to open settings under \"%1\": ").arg(m_settingsFile.fileName()) + m_settingsFile.errorString());
        return false;
    }
    return true;
}

bool App::loadSettings()
{
    if (!m_settingsFile.isOpen() && !openSettings()) {
        return false;
    }
    auto parsError = QJsonParseError();
    auto doc = QJsonDocument::fromJson(m_settingsFile.readAll(), &parsError);
    m_settings = doc.object();
    if (m_settingsFile.error() != QFile::NoError) {
        emit error(tr("Unable to read settings: ") + m_settingsFile.errorString());
        return false;
    }
    if (parsError.error != QJsonParseError::NoError || !doc.isObject()) {
        emit error(tr("Unable to restore settings: ")
            + (parsError.error != QJsonParseError::NoError ? parsError.errorString() : tr("JSON document contains no object")));
        return false;
    }
    emit settingsChanged(m_settings);
    return true;
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
    return true;
}

bool App::applySettings()
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
        handleGuiAddressChanged(m_launcher.guiUrl());
    } else if (m_connection.applySettings(m_connectionSettingsFromConfig) || !m_connection.isConnected()) {
        m_connection.reconnect();
    }
    auto tweaksSettings = m_settings.value(QLatin1String("tweaks")).toObject();
    m_alwaysUnloadGuiWhenHidden = tweaksSettings.value(QLatin1String("unloadGuiWhenHidden")).toBool(false);
    applyLauncherSettings();
    invalidateStatus();
    return true;
}

/// \cond
static void ensureDefault(bool &mod, QJsonObject &o, QLatin1String member, const QJsonValue &d)
{
    if (!o.contains(member)) {
        o.insert(member, d);
        mod = true;
    }
}
/// \endcond

void App::applyLauncherSettings()
{
    auto launcherSettings = m_settings.value(QLatin1String("launcher"));
    auto launcherSettingsObj = launcherSettings.toObject();
    auto mod = false;
    ensureDefault(mod, launcherSettingsObj, QLatin1String("run"), false);
    ensureDefault(mod, launcherSettingsObj, QLatin1String("stopOnMetered"), false);
    ensureDefault(mod, launcherSettingsObj, QLatin1String("writeLogFile"), false);
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    ensureDefault(mod, launcherSettingsObj, QLatin1String("logLevel"), m_launcher.libSyncthingLogLevelString());
#endif
    if (mod) {
        m_settings.insert(QLatin1String("launcher"), launcherSettingsObj);
    }

    m_launcher.setStoppingOnMeteredConnection(launcherSettingsObj.value(QLatin1String("stopOnMetered")).toBool());

    auto shouldRun = launcherSettingsObj.value(QLatin1String("run")).toBool();
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    auto options = LibSyncthing::RuntimeOptions();
    m_syncthingConfigDir = m_settingsDir->path() + QStringLiteral("/syncthing");
    m_syncthingDataDir = m_syncthingConfigDir;
    options.configDir = m_syncthingConfigDir.toStdString();
    options.dataDir = options.configDir;
    m_launcher.setLibSyncthingLogLevel(launcherSettingsObj.value(QLatin1String("logLevel")).toString());
    if (launcherSettingsObj.value(QLatin1String("writeLogFile")).toBool()) {
        if (!m_launcher.logFile().isOpen()) {
            m_launcher.logFile().setFileName(m_settingsDir->path() + QStringLiteral("/syncthing.log"));
            m_launcher.logFile().open(QIODeviceBase::WriteOnly | QIODeviceBase::Append | QIODeviceBase::Text);
        }
    } else {
        m_launcher.logFile().close();
    }
    m_launcher.setRunning(shouldRun, std::move(options));
#else
    if (shouldRun) {
        emit error(tr("This build of the app cannot launch Syncthing."));
    }
#endif
}

bool App::clearLogfile()
{
    auto launcherSettings = m_settings.value(QLatin1String("launcher"));
    auto launcherSettingsObj = launcherSettings.toObject();
    if (launcherSettingsObj.value(QLatin1String("writeLogFile")).toBool()) {
        launcherSettingsObj.insert(QLatin1String("writeLogFile"), false);
        m_settings.insert(QLatin1String("launcher"), launcherSettingsObj);
        applyLauncherSettings();
    }
    emit settingsChanged(m_settings);
    if (!storeSettings()) {
        return false;
    }

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
 * \brief Checks the location specified via \a url for settings to import.
 */
bool App::checkSettings(const QUrl &url, const QJSValue &callback)
{
    if (checkOngoingImportExport()) {
        return false;
    }
    setImportExportStatus(ImportExportStatus::Checking);
    QtConcurrent::run([this, url] {
        auto availableSettings = QVariantMap();
        auto pathStr = resolveUrl(url);
        auto path = std::filesystem::path(SYNCTHING_APP_STRING_CONVERSION(pathStr));
        auto errors = QStringList();
        availableSettings.insert(QStringLiteral("path"), pathStr);
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
            callback.call(QJSValueList{ m_engine.toScriptValue(availableSettings) });
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

    // stop Syncthing if necassary
    const auto importSyncthingHome = selectedSettings.value(QStringLiteral("syncthingHome")).toBool();
    if (importSyncthingHome && m_launcher.isRunning()) {
        emit info(tr("Waiting for backend to terminate before importing settings …"));
        m_settingsImport.first = availableSettings;
        m_settingsImport.second = selectedSettings;
        m_launcher.terminate();
        return false;
    }

    setImportExportStatus(ImportExportStatus::Importing);

    QtConcurrent::run([this, importSyncthingHome, availableSettings, selectedSettings, rawConfig = m_connection.rawConfig()] () mutable {
        // copy selected files from import directory to settings directory
        auto summary = QStringList();
        try {
            // copy app config file
            const auto settingsPath = std::filesystem::path(SYNCTHING_APP_STRING_CONVERSION(m_settingsDir->path()));
            const auto importAppConfig = selectedSettings.value(QStringLiteral("appConfig")).toBool();
            if (importAppConfig) {
                const auto appConfigSrcPathStr = availableSettings.value(QStringLiteral("appConfigPath")).toString();
                const auto appConfigSrcPath = SYNCTHING_APP_STRING_CONVERSION(appConfigSrcPathStr);
                const auto appConfigDstPath = settingsPath / "appconfig.json";
                std::filesystem::copy_file(appConfigSrcPath, appConfigDstPath, std::filesystem::copy_options::overwrite_existing);
                summary.append(tr("Imported app config from \"%1\".").arg(appConfigSrcPathStr));
            }

            // copy Syncthing home directory
            if (importSyncthingHome) {
                const auto homeSrcPathStr = availableSettings.value(QStringLiteral("syncthingHomePath")).toString();
                const auto homeSrcPath = SYNCTHING_APP_STRING_CONVERSION(homeSrcPathStr);
                const auto homeDstPath = settingsPath / "syncthing";
                std::filesystem::remove_all(homeDstPath);
                std::filesystem::create_directory(homeDstPath);
                std::filesystem::copy(
                    homeSrcPath, homeDstPath, std::filesystem::copy_options::update_existing | std::filesystem::copy_options::recursive);
                summary.append(tr("Imported Syncthing config and database from \"%1\".").arg(homeSrcPathStr));
            }
        } catch (const std::filesystem::filesystem_error &e) {
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

    QtConcurrent::run([this, url] {
        const auto path = resolveUrl(url);
        const auto dir = QDir(path);
        if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
            return std::make_pair(tr("unable to create export directory under \"%1\"").arg(path), true);
        }
        if (!m_settingsDir.has_value()) {
            return std::make_pair(tr("settings directory was not located."), true);
        }
        auto ec = std::error_code();
        std::filesystem::copy(SYNCTHING_APP_STRING_CONVERSION(m_settingsDir->path()), SYNCTHING_APP_STRING_CONVERSION(path),
            std::filesystem::copy_options::update_existing | std::filesystem::copy_options::recursive, ec);
        if (ec) {
            emit error(tr("Unable to export settings: %1").arg(QString::fromStdString(ec.message())));
            return std::make_pair(QString::fromStdString(ec.message()), true);
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
