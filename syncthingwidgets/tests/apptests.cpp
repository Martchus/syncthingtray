#include "../quick/app.h"

#include <QtQuickTest>
#include <QLocale>
#include <QQmlEngine>
#include <QQmlContext>
#include <QGuiApplication>

#include <optional>

class Setup : public QObject
{
    Q_OBJECT

public:
    Setup() {}

public Q_SLOTS:
    void applicationAvailable()
    {
        QLocale::setDefault(QLocale::English);
    }

    void qmlEngineAvailable(QQmlEngine *engine)
    {
        m_app.emplace(true, engine);
    }

    void cleanupTestCase()
    {
        m_app.reset();
    }

private:
    std::optional<QtGui::App> m_app;
};

QUICK_TEST_MAIN_WITH_SETUP(apptest, Setup)

#include "apptests.moc"
