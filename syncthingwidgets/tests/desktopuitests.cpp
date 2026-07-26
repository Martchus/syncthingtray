#define SYNCTHINGTESTHELPER_FOR_CLI

#include "./testhelper.h"

#include "../misc/syncthingdata.h"
#include "../misc/syncthingmodels.h"
#include "../quick/helpers.h"
#include "../quick/quickui.h"

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

using namespace QtGui;

class MockTrayWidget : public QObject {
    Q_OBJECT
public:
    explicit MockTrayWidget(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

public Q_SLOTS:
    void handleMainWindowVisibleChanged(bool visible)
    {
        qDebug() << "MockTrayWidget::handleMainWindowVisibleChanged" << visible;
    }
    void showAboutDialog()
    {
        qDebug() << "MockTrayWidget::showAboutDialog";
    }
    void showSyncthingUI(bool flag)
    {
        qDebug() << "MockTrayWidget::showSyncthingUI" << flag;
    }
    void showOwnDeviceId()
    {
        qDebug() << "MockTrayWidget::showOwnDeviceId";
    }
    void showLog()
    {
        qDebug() << "MockTrayWidget::showLog";
    }
    void showSettingsDialog()
    {
        qDebug() << "MockTrayWidget::showSettingsDialog";
    }
    void showWizard()
    {
        qDebug() << "MockTrayWidget::showWizard";
    }
    void handleCurrentTabChanged(int index)
    {
        qDebug() << "MockTrayWidget::handleCurrentTabChanged" << index;
        emit currentTabChanged(index);
    }
    void closeMenu()
    {
        qDebug() << "MockTrayWidget::closeMenu";
    }
    bool showFileBrowser(const QString &dirId)
    {
        qDebug() << "MockTrayWidget::showFileBrowser" << dirId;
        return true;
    }

Q_SIGNALS:
    void currentTabChanged(int index);
};

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
        prepareTestEnvironment(m_settingsDir, m_exportDir, m_syncthingPath, m_testConfigDir, m_withSyncthing
            /*, "SYNCTHINGWIDGETS_DESKTOP_UI_TESTS_WITH_SYNCTHING" */
        );
    }

    void qmlEngineAvailable(QQmlEngine *engine)
    {
        qmlRegisterSingletonInstance<MockTrayWidget>("Tray", 1, 0, "TrayWidget", new MockTrayWidget(engine));

        registerCommonContextProperties(engine, false, m_settingsDir.path(), m_testConfigDir, m_exportDir.path(), this);

        m_data.emplace(nullptr);
        m_models.emplace(*m_data, engine);

        m_quickUI.emplace(qGuiApp, Settings::values().qt);
        m_quickUI->ui.setEngine(engine);
        m_quickUI->ui.setSyncthingIconsVisible(true);

        dataObjectToProperty(engine, &*m_data);
        dataObjectToProperty(engine, &*m_models);
        dataObjectToProperty(engine, &m_quickUI->ui);
    }

    void cleanupTestCase()
    {
        m_quickUI.reset();
        m_models.reset();
        m_data.reset();
    }

private:
    TestApplication m_testapp;
    std::optional<QtGui::SyncthingData> m_data;
    std::optional<QtGui::SyncthingModels> m_models;
    std::optional<QtGui::QuickGuiEngine> m_quickUI;
    // not used at this point
    QTemporaryDir m_settingsDir;
    QTemporaryDir m_exportDir;
    QString m_testConfigDir;
    QString m_syncthingPath;
    bool m_withSyncthing = false;
};

QT_UTILITIES_DISABLE_WARNINGS_FOR_MOC_INCLUDE
QUICK_TEST_MAIN_WITH_SETUP(desktopuitest, Setup)
#include "desktopuitests.moc"
