#include "./app.h"

#include <qtforkawesome/renderer.h>

#include <qtquickforkawesome/imageprovider.h>

#include <QQmlContext>
#include <QGuiApplication>

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

} // namespace QtGui
