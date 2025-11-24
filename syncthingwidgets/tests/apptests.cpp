#include "./testhelper.h"

#include "../quick/app.h"

#include <QtQuickTest>
#include <QQmlEngine>
#include <QQmlContext>
#include <QGuiApplication>
#include <QTemporaryDir>

#include <optional>

class Setup : public QObject
{
    Q_OBJECT

public:
    Setup() {}

public Q_SLOTS:
    void applicationAvailable()
    {
        initTestLocale();
        initTestSettings(Settings::values());
        initTestHomeDir(m_homeDir);
        initTestConfig();
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
    QTemporaryDir m_homeDir;
};

QUICK_TEST_MAIN_WITH_SETUP(apptest, Setup)

#include "apptests.moc"
