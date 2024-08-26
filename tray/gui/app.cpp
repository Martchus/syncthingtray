#include "./app.h"

#include <syncthingmodel/syncthingfilemodel.h>
#include <syncthingmodel/syncthingicons.h>

#include <qtutilities/misc/desktoputils.h>

#include <qtforkawesome/renderer.h>
#include <qtquickforkawesome/imageprovider.h>

#include <QClipboard>
#include <QDir>
#include <QGuiApplication>
#include <QQmlContext>

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

bool App::openDir(const QString &path)
{
    if (QDir(path).exists()) {
        QtUtilities::openLocalFileOrDir(path);
        return true;
    }
    return false;
}

bool App::copy(const QString &text)
{
    if (auto *const clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(text);
        return true;
    }
    return false;
}

SyncthingFileModel *App::createFileModel(const QString &dirId)
{
    auto row = int();
    auto dirInfo = m_connection.findDirInfo(dirId, row);
    return dirInfo ? new Data::SyncthingFileModel(m_connection, *dirInfo, this) : nullptr;
}

} // namespace QtGui
