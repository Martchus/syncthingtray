#include "./app.h"

#include "resources/config.h"

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
#include <syncthing/interface.h>
#endif

#include <syncthingwidgets/misc/otherdialogs.h>
#include <syncthingwidgets/settings/settings.h>

#include <syncthingmodel/syncthingfilemodel.h>
#include <syncthingmodel/syncthingicons.h>

#include <syncthingconnector/syncthingconnectionsettings.h>

#include <qtutilities/misc/desktoputils.h>

#include <qtforkawesome/renderer.h>
#include <qtquickforkawesome/imageprovider.h>

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

#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>

#ifdef Q_OS_WINDOWS
#define SYNCTHING_APP_STRING_CONVERSION(s) s.toStdWString()
#else
#define SYNCTHING_APP_STRING_CONVERSION(s) s.toLocal8Bit().toStdString()
#endif

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

using namespace Data;

namespace QtGui {

App::App(bool insecure, QObject *parent)
    : QObject(parent)
    , m_notifier(m_connection)
    , m_dirModel(m_connection)
    , m_devModel(m_connection)
    , m_changesModel(m_connection)
    , m_faUrlBase(QStringLiteral("image://fa/"))
#ifdef Q_OS_ANDROID
    , m_iconSize(8)
#else
    , m_iconSize(16)
#endif
    , m_insecure(insecure)
    , m_darkmodeEnabled(false)
    , m_darkColorScheme(false)
    , m_darkPalette(QtUtilities::isPaletteDark())
{
    qmlRegisterUncreatableType<Data::SyncthingFileModel>(
        "Main.Private", 1, 0, "SyncthingFileModel", QStringLiteral("Data::SyncthingFileModel is created from C++."));
    qmlRegisterUncreatableType<QtGui::DiffHighlighter>(
        "Main.Private", 1, 0, "DiffHighlighter", QStringLiteral("QtGui::DiffHighlighter is created from C++."));

    loadSettings();
    applySettings();
    QtUtilities::onDarkModeChanged([this](bool darkColorScheme) { applyDarkmodeChange(darkColorScheme, m_darkPalette); }, this);

    auto statusIconSettings = StatusIconSettings();
    statusIconSettings.strokeWidth = StatusIconStrokeWidth::Thick;
    Data::IconManager::instance().applySettings(&statusIconSettings, &statusIconSettings, true, true);

    m_connection.setPollingFlags(SyncthingConnection::PollingFlags::MainEvents | SyncthingConnection::PollingFlags::Errors);
    m_notifier.setEnabledNotifications(Data::SyncthingHighLevelNotification::ConnectedDisconnected);
    connect(&m_connection, &SyncthingConnection::error, this, &App::handleConnectionError);
    connect(&m_connection, &SyncthingConnection::statusChanged, this, &App::invalidateStatus);

    m_launcher.setEmittingOutput(true);
    connect(&m_launcher, &SyncthingLauncher::outputAvailable, this, &App::gatherLogs);

    auto *const app = QGuiApplication::instance();
    auto *const context = m_engine.rootContext();
    context->setContextProperty(QStringLiteral("app"), this);
    connect(
        &m_engine, &QQmlApplicationEngine::objectCreated, app,
        [](QObject *obj, const QUrl &objUrl) {
            if (!obj) {
                std::cerr << "Unable to load " << objUrl.toString().toStdString() << '\n';
                QCoreApplication::exit(EXIT_FAILURE);
            }
        },
        Qt::QueuedConnection);
    connect(&m_engine, &QQmlApplicationEngine::quit, app, &QGuiApplication::quit);
    m_engine.addImageProvider(QStringLiteral("fa"), new QtForkAwesome::QuickImageProvider(QtForkAwesome::Renderer::global()));
    loadMain();
}

QString App::status()
{
    if (m_status.has_value()) {
        return *m_status;
    }
    switch (m_connection.status()) {
    case Data::SyncthingStatus::Disconnected:
        return m_status.emplace(tr("Not connected to backend. Check app settings for problems."));
    case Data::SyncthingStatus::Reconnecting:
        return m_status.emplace(tr("Waiting for backend …"));
    default:
        return m_status.emplace();
    }
}

bool App::loadMain()
{
    // allow overriding Qml entry point for hot-reloading; otherwise load proper Qml module from resources
    if (const auto path = qEnvironmentVariable(PROJECT_VARNAME_UPPER "_QML_MAIN_PATH"); !path.isEmpty()) {
        qDebug() << "Path Qml entry point for Qt Quick GUI was overriden to: " << path;
        m_engine.load(path);
    } else {
        m_engine.loadFromModule("Main", "Main");
    }

    // set window icon
    for (auto *const rootObject : m_engine.rootObjects()) {
        if (auto *const rootWindow = qobject_cast<QQuickWindow *>(rootObject)) {
            rootWindow->setIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
        }
    }
    return true;
}

bool App::reload()
{
    qDebug() << "Reloading Qt Quick GUI";
    for (auto *const rootObject : m_engine.rootObjects()) {
        rootObject->deleteLater();
    }
    m_engine.clearComponentCache();
    return loadMain();
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

bool App::copyText(const QString &text)
{
    if (auto *const clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(text);
        emit info(tr("Copied value"));
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
        emit info(tr("Copied path"));
        return true;
    }
    return false;
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

bool App::showLog(QObject *textArea)
{
    textArea->setProperty("text", m_log);
    connect(this, &App::logsAvailable, textArea, [textArea](const QString &newLogs) {
        QMetaObject::invokeMethod(textArea, "insert", Q_ARG(int, textArea->property("length").toInt()), Q_ARG(QString, newLogs));
    });
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

bool App::showError(const QString &errorMessage)
{
    emit error(errorMessage);
    return true;
}

SyncthingFileModel *App::createFileModel(const QString &dirId, QObject *parent)
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

bool App::event(QEvent *event)
{
    const auto res = QObject::event(event);
    switch (event->type()) {
    case QEvent::ApplicationPaletteChange:
        qDebug() << "application pallette has changed";
        applyDarkmodeChange(m_darkColorScheme, QtUtilities::isPaletteDark());
        break;
    default:;
    }
    return res;
}

void App::handleConnectionError(
    const QString &errorMessage, Data::SyncthingErrorCategory category, int networkError, const QNetworkRequest &request, const QByteArray &response)
{
    Q_UNUSED(category)
    Q_UNUSED(networkError)
    Q_UNUSED(request)
    Q_UNUSED(response)
    qWarning() << "connection error: " << errorMessage;
}

void App::setCurrentControls(bool visible, int tabIndex)
{
    auto flags = m_connection.pollingFlags();
    //CppUtilities::modFlagEnum(flags, Data::SyncthingConnection::PollingFlags::TrafficStatistics, visible && tabIndex == 0);
    CppUtilities::modFlagEnum(flags, Data::SyncthingConnection::PollingFlags::DeviceStatistics, visible && tabIndex == 1);
    CppUtilities::modFlagEnum(flags, Data::SyncthingConnection::PollingFlags::DiskEvents, visible && tabIndex == 2);
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
#ifdef Q_OS_ANDROID
    const auto urlString = url.toString(QUrl::FullyEncoded);
    const auto path = QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jstring>("resolveUri", urlString).toString();
    if (path.isEmpty()) {
        showToast(tr("Unable to resolve URL \"%1\".").arg(urlString));
    }
    return path.isEmpty() ? urlString : path;
#else
    return url.path();
#endif
}

void App::invalidateStatus()
{
    m_status.reset();
    emit statusChanged();
}

void App::gatherLogs(const QByteArray &newOutput)
{
    const auto asText = QString::fromUtf8(newOutput);
    emit logsAvailable(asText);
    m_log.append(asText);
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
    if (connectionSettings.isObject()) {
        couldLoadCertificate = m_connectionSettings.loadFromJson(connectionSettingsObj);
    } else {
        m_connectionSettings.storeToJson(connectionSettingsObj);
        m_settings.insert(QLatin1String("connection"), connectionSettingsObj);
        couldLoadCertificate = m_connectionSettings.loadHttpsCert();
    }
    if (!couldLoadCertificate) {
        emit error(tr("Unable to load HTTPs certificate"));
    }
    m_connection.setInsecure(m_insecure);
    m_connection.connect(m_connectionSettings);
    applyLauncherSettings();
    return true;
}

void App::applyLauncherSettings()
{
    auto launcherSettings = m_settings.value(QLatin1String("launcher"));
    auto launcherSettingsObj = launcherSettings.toObject();
    if (!launcherSettings.isObject()) {
        launcherSettingsObj.insert(QLatin1String("run"), false);
        launcherSettingsObj.insert(QLatin1String("stopOnMetered"), false);
        m_settings.insert(QLatin1String("launcher"), launcherSettingsObj);
    }

    m_launcher.setStoppingOnMeteredConnection(launcherSettingsObj.value(QLatin1String("stopOnMetered")).toBool());

    auto shouldRun = launcherSettingsObj.value(QLatin1String("run")).toBool();
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    auto options = LibSyncthing::RuntimeOptions();
    options.configDir = (m_settingsDir->path() + QStringLiteral("/syncthing/config")).toStdString();
    options.dataDir = (m_settingsDir->path() + QStringLiteral("/syncthing/data")).toStdString();
    m_launcher.setRunning(shouldRun, std::move(options));
#else
    if (shouldRun) {
        emit error(tr("This build of the app cannot launch Syncthing."));
    }
#endif
}

bool App::importSettings(const QUrl &url)
{
    if (!m_settingsDir.has_value()) {
        emit error(tr("Unable to import settings: settings directory was not located."));
        return false;
    }
    const auto path = resolveUrl(url);
    auto ec = std::error_code();
    std::filesystem::copy(SYNCTHING_APP_STRING_CONVERSION(path), SYNCTHING_APP_STRING_CONVERSION(m_settingsDir->path()),
        std::filesystem::copy_options::update_existing | std::filesystem::copy_options::recursive, ec);
    if (ec) {
        emit error(tr("Unable to import settings: %1").arg(QString::fromStdString(ec.message())));
        return false;
    }
    emit info(tr("Settings have been imported from \"%1\".").arg(path));
    m_settingsFile.close();
    return loadSettings();
}

bool App::exportSettings(const QUrl &url)
{
    const auto path = resolveUrl(url);
    const auto dir = QDir(path);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        emit error(tr("Unable to create export directory under \"%1\".").arg(path));
        return false;
    }
    if (!m_settingsDir.has_value()) {
        emit error(tr("Unable to export settings: settings directory was not located."));
        return false;
    }
    auto ec = std::error_code();
    std::filesystem::copy(SYNCTHING_APP_STRING_CONVERSION(m_settingsDir->path()), SYNCTHING_APP_STRING_CONVERSION(path),
        std::filesystem::copy_options::update_existing | std::filesystem::copy_options::recursive, ec);
    if (ec) {
        emit error(tr("Unable to export settings: %1").arg(QString::fromStdString(ec.message())));
        return false;
    }
    emit info(tr("Settings have been exported to \"%1\".").arg(path));
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
    qDebug() << "darkmode has changed: " << isDarkmodeEnabled;
    m_darkmodeEnabled = isDarkmodeEnabled;
    m_dirModel.setBrightColors(isDarkmodeEnabled);
    m_devModel.setBrightColors(isDarkmodeEnabled);
    m_changesModel.setBrightColors(isDarkmodeEnabled);
    emit darkmodeEnabledChanged(isDarkmodeEnabled);
}

} // namespace QtGui
