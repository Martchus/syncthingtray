#include "./syncthingmodels.h"

namespace QtGui {

SyncthingData::SyncthingData(QObject *parent)
    : QObject(parent)
    , m_notifier(m_connection)
{
    m_connection.setPollingFlags(SyncthingConnection::PollingFlags::MainEvents | SyncthingConnection::PollingFlags::Errors);
}

SyncthingData::~SyncthingData()
{
}

SyncthingModels::SyncthingModels(SyncthingData &data, QObject *parent)
    : QObject(parent)
    , m_dirModel(data.connection())
    , m_sortFilterDirModel(&m_dirModel)
    , m_devModel(data.connection())
    , m_sortFilterDevModel(&m_devModel)
    , m_recentChangesModel(data.connection())
{
}

SyncthingModels::~SyncthingModels()
{
}

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
template <typename SyncthingClass> static SyncthingClass *dataObjectFromProperty(QQmlEngine *qmlEngine, QJSEngine *engine, const char *propertyName)
{
    Q_UNUSED(qmlEngine)
    auto *const dataObject = engine->property(propertyName).value<SyncthingClass *>();
    QJSEngine::setObjectOwnership(dataObject, QJSEngine::CppOwnership);
    return dataObject;
}

SyncthingData *SyncthingData::create(QQmlEngine *qmlEngine, QJSEngine *engine)
{
    return dataObjectFromProperty<SyncthingData>(qmlEngine, engine, "connector");
}

SyncthingModels *SyncthingModels::create(QQmlEngine *qmlEngine, QJSEngine *engine)
{
    return dataObjectFromProperty<SyncthingModels>(qmlEngine, engine, "models");
}
#endif

} // namespace QtGui
