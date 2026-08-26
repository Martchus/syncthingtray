#define SYNCTHINGTESTHELPER_FOR_CLI

#include "./testhelper.h"

#include "../misc/syncthingdata.h"
#include "../misc/syncthingmodels.h"
#include "../quick/helpers.h"
#include "../quick/quickui.h"

#include <qtutilities/misc/disablewarningsmoc.h>

#include <QtQuickTest>

#include <optional>

using namespace QtGui;

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
    }

    void qmlEngineAvailable(QQmlEngine *engine)
    {
        auto *const context = engine->rootContext();
        context->setContextProperty(QStringLiteral("setup"), this);

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
    }

private:
    std::optional<QtGui::SyncthingData> m_data;
    std::optional<QtGui::SyncthingModels> m_models;
    std::optional<QtGui::QuickGuiEngine> m_quickUI;
};

QT_UTILITIES_DISABLE_WARNINGS_FOR_MOC_INCLUDE
QUICK_TEST_MAIN_WITH_SETUP(basicquicktest, Setup)
#include "basicquicktests.moc"
