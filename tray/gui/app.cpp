#include "./app.h"

#include "resources/config.h"

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
#include <functional>
#include <iostream>

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

    connect(&m_connection, &SyncthingConnection::error, this, &App::handleConnectionError);

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
    if (QFile::exists(path) && QtUtilities::openLocalFileOrDir(path)) {
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

bool App::openSettings()
{
    const auto dir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
    m_settingsFile.close();
    m_settingsFile.unsetError();
    m_settingsFile.setFileName(dir.path() + QStringLiteral("/appconfig.json"));
    if (!m_settingsFile.open(QFile::ReadWrite)) {
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
    auto connectionSettingsObj = QJsonObject();
    if (!connectionSettings.isObject()) {
        m_connectionSettings.storeToJson(connectionSettingsObj);
        connectionSettings = *m_settings.insert(QLatin1String("connection"), connectionSettingsObj);
    } else {
        connectionSettingsObj = connectionSettings.toObject();
    }
    if (!m_connectionSettings.loadFromJson(connectionSettings.toObject())) {
        emit error(tr("Unable to load HTTPs certificate"));
    }
    m_connectionSettings.storeToJson(connectionSettingsObj);
    m_settings.insert(QLatin1String("connection"), connectionSettingsObj);
    m_connection.setInsecure(m_insecure);
    m_connection.connect(m_connectionSettings);
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
