#include "./app.h"

#include <syncthingwidgets/settings/settings.h>

#include <syncthingmodel/syncthingfilemodel.h>
#include <syncthingmodel/syncthingicons.h>

#include <qtutilities/misc/desktoputils.h>

#include <qtforkawesome/renderer.h>
#include <qtquickforkawesome/imageprovider.h>

#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QNetworkReply>
#include <QQmlContext>
#include <QStringBuilder>

#include <cstdlib>
#include <iostream>

using namespace Data;

namespace QtGui {

App::App(QObject *parent)
    : QObject(parent)
    , m_dirModel(m_connection)
    , m_devModel(m_connection)
    , m_changesModel(m_connection)
    , m_faUrlBase(QStringLiteral("image://fa/"))
{
    qmlRegisterUncreatableType<Data::SyncthingFileModel>(
        "Main.Private", 1, 0, "SyncthingFileModel", QStringLiteral("Data::SyncthingFileModel is created from C++."));

    setBrightColorsOfModelsAccordingToPalette();
    Data::IconManager::instance().applySettings(nullptr, nullptr, true, true);

    auto *const app = QGuiApplication::instance();
    auto *const context = m_engine.rootContext();
    context->setContextProperty(QStringLiteral("app"), this);
    QObject::connect(
        &m_engine, &QQmlApplicationEngine::objectCreated, app,
        [](QObject *obj, const QUrl &objUrl) {
            if (!obj) {
                std::cerr << "Unable to load " << objUrl.toString().toStdString() << '\n';
                QCoreApplication::exit(EXIT_FAILURE);
            }
        },
        Qt::QueuedConnection);
    QObject::connect(&m_engine, &QQmlApplicationEngine::quit, app, &QGuiApplication::quit);
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
        setBrightColorsOfModelsAccordingToPalette();
        break;
    default:;
    }
    return res;
}

void App::setBrightColorsOfModelsAccordingToPalette()
{
    auto &qtSettings = Settings::values().qt;
    qtSettings.reevaluatePaletteAndDefaultIconTheme();
    const auto brightColors = qtSettings.isPaletteDark();
    m_dirModel.setBrightColors(brightColors);
    m_devModel.setBrightColors(brightColors);
    m_changesModel.setBrightColors(brightColors);
}

} // namespace QtGui
