#include "./app.h"

#include <qtforkawesome/renderer.h>

#include <qtquickforkawesome/imageprovider.h>

#include <QQmlContext>
#include <QGuiApplication>

#include <QDir>

#include <QFile>
#include <QFileSystemWatcher>
#include <QKeySequence>
#include <QShortcut>
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
{
    auto *const app = QGuiApplication::instance();
    auto *const context = m_engine.rootContext();
    context->setContextProperty(QStringLiteral("app"), this);

    // setup hot-reloading
    auto hotReloadEnv = qEnvironmentVariable("SYNCTHING_TRAY_APP_HOT_RELOAD");
    if (!hotReloadEnv.isEmpty()) {
        auto srcdirref = QFile(QCoreApplication::applicationDirPath() + QStringLiteral("/srcdirref"));
        srcdirref.open(QFile::ReadOnly);
        auto srcdir = srcdirref.readLine();
        auto qmldir = QDir(QString::fromUtf8(srcdir.removeLast().removeLast()) + QStringLiteral("/gui/qml"));
        auto qmldirPath = qmldir.absolutePath();
        if (srcdir.isEmpty() || !qmldir.exists()) {
            std::cerr << "Unable to determine QML directory for hot-reload.\n";
            std::cerr << "Assumed directory: " << qmldirPath.toStdString() << '\n';
            std::exit(EXIT_FAILURE);
        }
        m_engine.addImportPath(qmldirPath);

        auto hotReloadShortcut = new QShortcut(QKeySequence(hotReloadEnv), this);
        connect(hotReloadShortcut, &QShortcut::activated, this, &App::reload);
        connect(hotReloadShortcut, &QShortcut::activatedAmbiguously, this, &App::reload);

        auto watcher = new QFileSystemWatcher(this);
        auto paths = qmldir.entryList(QDir::Files | QDir::NoDotAndDotDot);
        for (const auto &path : paths) {
            watcher->addPath(qmldirPath % QChar('/') % path);
        }
        connect(watcher, &QFileSystemWatcher::fileChanged, this, &App::reload);
    }

    // setup error handling and quitting
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

void App::reload()
{
    std::cerr << "Reloading QML engine.\n";
    m_engine.clearComponentCache();
}

} // namespace QtGui
