#define SYNCTHINGTESTHELPER_FOR_CLI

#include "./testhelper.h"

#include "../quick/app.h"
#include "../quick/appservice.h"
#include "../quick/helpers.h"

#include "../../testhelper/helper.h"

#include <qtutilities/misc/disablewarningsmoc.h>

#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTemporaryDir>
#include <QtQuickTest>

#include <optional>

class Setup : public QObject {
    Q_OBJECT

public:
    Setup()
    {
    }

public Q_SLOTS:
    void debug(const QString &context, const QString &message)
    {
        qDebug() << context.toStdString().data() << message;
    }

    void applicationAvailable()
    {
        prepareTestEnvironment(
            m_settingsDir, m_exportDir, m_syncthingPath, m_testConfigDir, m_withSyncthing, "SYNCTHINGWIDGETS_APP_TESTS_WITH_SYNCTHING");
    }

    void qmlEngineAvailable(QQmlEngine *engine)
    {
        registerCommonContextProperties(engine, m_withSyncthing, m_settingsDir.path(), m_testConfigDir, m_exportDir.path(), this);

        m_service.emplace(true);
        m_app.emplace(true, engine);
        QCOMPARE(m_service->settingsDir().path(), m_settingsDir.path());
        QCOMPARE(m_app->settingsDir().path(), m_settingsDir.path());
        QtGui::connectAppAndService(*m_app, *m_service);

        if (m_withSyncthing) {
            qDebug() << "Waiting for service and app to connect to Syncthing";
            QVERIFY(waitForConnected(*m_service->data()->connection(), 15000));
            QVERIFY(waitForConnected(*m_app->data()->connection(), 15000));
            qDebug() << "Service connected to Syncthing: " << m_service->data()->connection()->syncthingVersion();
            qDebug() << "App connected to Syncthing: " << m_app->data()->connection()->syncthingVersion();
            m_syncthingVersion = m_service->data()->connection()->syncthingVersion();
        } else {
            qDebug() << "Starting test without Syncthing";
        }
    }

    void checkExport()
    {
        qDebug() << "Checking exported files under: " << m_exportDir.path();
        auto expectedFiles = QStringList();
        expectedFiles.reserve(8);
        expectedFiles.append(QStringList{
            QStringLiteral("appconfig.json"),
        });
        if (m_withSyncthing) {
            expectedFiles.append(QStringList{
                QStringLiteral("syncthing/cert.pem"),
                QStringLiteral("syncthing/config.xml"),
                QStringLiteral("syncthing/https-cert.pem"),
                QStringLiteral("syncthing/https-key.pem"),
                QStringLiteral("syncthing/key.pem"),
            });
            if (m_syncthingVersion.startsWith(QLatin1String("v2"))) {
                expectedFiles.append(QStringList{
                    QStringLiteral("syncthing/index-v2"),
                    QStringLiteral("syncthing/index-v2/main.db"),
                });
            }
        }
        for (const auto &fileName : std::as_const(expectedFiles)) {
            const auto filePath = m_exportDir.filePath(fileName);
            QVERIFY2(QFileInfo(filePath).size() > 0, (filePath.toStdString() + " was exported").data());
        }
    }

    void cleanupTestCase()
    {
        m_service.reset();
        m_app.reset();
    }

private:
    TestApplication m_testapp;
    std::optional<QtGui::App> m_app;
    std::optional<QtGui::AppService> m_service;
    QTemporaryDir m_settingsDir;
    QTemporaryDir m_exportDir;
    QString m_testConfigDir;
    QString m_syncthingPath;
    QString m_syncthingVersion;
    bool m_withSyncthing = false;
};

QT_UTILITIES_DISABLE_WARNINGS_FOR_MOC_INCLUDE
QUICK_TEST_MAIN_WITH_SETUP(apptest, Setup)
#include "apptests.moc"
