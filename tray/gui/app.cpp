#include "./app.h"

#include <syncthingwidgets/settings/settings.h>

#include <syncthingmodel/syncthingfilemodel.h>
#include <syncthingmodel/syncthingicons.h>

#include <syncthingconnector/syncthingconnectionsettings.h>

#include <qtutilities/misc/desktoputils.h>

#include <qtforkawesome/renderer.h>
#include <qtquickforkawesome/imageprovider.h>

#include <QClipboard>
#include <QDebug>
#include <QFile>
#include <QGuiApplication>
#include <QNetworkReply>
#include <QQmlContext>
#include <QStringBuilder>

#include <cstdlib>
#include <functional>
#include <iostream>

using namespace Data;

namespace QtGui {

AppSettings::AppSettings(Data::SyncthingConnectionSettings &connectionSettings, QObject *parent)
    : QObject(parent)
    , syncthingUrl(connectionSettings.syncthingUrl)
    , apiKey(connectionSettings.apiKey)
{
}

QString AppSettings::apiKeyAsString() const
{
    return QString::fromUtf8(apiKey);
}

void AppSettings::setApiKeyFromString(const QString &apiKeyAsString)
{
    apiKey = apiKeyAsString.toUtf8();
}

App::App(QObject *parent)
    : QObject(parent)
    , m_notifier(m_connection)
    , m_dirModel(m_connection)
    , m_devModel(m_connection)
    , m_changesModel(m_connection)
    , m_settings(Settings::values().connection.primary)
    , m_faUrlBase(QStringLiteral("image://fa/"))
    , m_darkmodeEnabled(false)
    , m_darkColorScheme(false)
    , m_darkPalette(QtUtilities::isPaletteDark())
{
    qmlRegisterUncreatableType<Data::SyncthingFileModel>(
        "Main.Private", 1, 0, "SyncthingFileModel", QStringLiteral("Data::SyncthingFileModel is created from C++."));

    QtUtilities::onDarkModeChanged([this] (bool darkColorScheme) {
        applyDarkmodeChange(darkColorScheme, m_darkPalette);
    }, this);
    Data::IconManager::instance().applySettings(nullptr, nullptr, true, true);

    connect(&m_settings, &AppSettings::settingsChanged, this, &App::applySettings);
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
    m_engine.loadFromModule("Main", "Main");
}

bool App::openPath(const QString &path)
{
    if (QFile::exists(path)) {
        QtUtilities::openLocalFileOrDir(path);
        return true;
    }
    return false;
}

bool App::openPath(const QString &dirId, const QString &relativePath)
{
    auto row = int();
    auto dirInfo = m_connection.findDirInfo(dirId, row);
    return dirInfo ? openPath(dirInfo->path % QChar('/') % relativePath) : false;
}

bool App::copyText(const QString &text)
{
    if (auto *const clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(text);
        return true;
    }
    return false;
}

bool App::copyPath(const QString &dirId, const QString &relativePath)
{
    auto row = int();
    auto dirInfo = m_connection.findDirInfo(dirId, row);
    return dirInfo ? copyText(dirInfo->path % QChar('/') % relativePath) : false;
}

bool App::loadIgnorePatterns(const QString &dirId, QObject *textArea)
{
    auto res = m_connection.ignores(dirId, [textArea](SyncthingIgnores &&ignores, QString &&error) {
        Q_UNUSED(error) // FIXME: error handling
        textArea->setProperty("text", ignores.ignore.join(QChar('\n')));
        textArea->setProperty("enabled", true);
    });
    connect(textArea, &QObject::destroyed, res.reply, &QNetworkReply::deleteLater);
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
        dirId, SyncthingIgnores{ .ignore = text.toString().split(QChar('\n')), .expanded = QStringList() }, [textArea](QString &&error) {
            Q_UNUSED(error) // FIXME: error handling
            textArea->setProperty("enabled", true);
        });
    connect(textArea, &QObject::destroyed, res.reply, &QNetworkReply::deleteLater);
    return true;
}

SyncthingFileModel *App::createFileModel(const QString &dirId, QObject *parent)
{
    auto row = int();
    auto dirInfo = m_connection.findDirInfo(dirId, row);
    return dirInfo ? new Data::SyncthingFileModel(m_connection, *dirInfo, parent) : nullptr;
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

void App::handleConnectionError(const QString &errorMessage, Data::SyncthingErrorCategory category, int networkError, const QNetworkRequest &request, const QByteArray &response)
{
    Q_UNUSED(category)
    Q_UNUSED(networkError)
    Q_UNUSED(request)
    Q_UNUSED(response)
    qWarning() << "connection error: " << errorMessage;
}

bool App::applySettings()
{
    auto &connectionSettings = Settings::values().connection;
    m_connection.setInsecure(connectionSettings.insecure);
    m_connection.connect(connectionSettings.primary);
    return true;
}

void App::applyDarkmodeChange(bool isDarkColorSchemeEnabled, bool isDarkPaletteEnabled)
{
    auto &qtSettings = Settings::values().qt;
    m_darkColorScheme = isDarkColorSchemeEnabled;
    m_darkPalette = isDarkPaletteEnabled;
    const auto isDarkmodeEnabled = m_darkColorScheme || m_darkPalette;
    qtSettings.reapplyDefaultIconTheme(isDarkmodeEnabled);
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
