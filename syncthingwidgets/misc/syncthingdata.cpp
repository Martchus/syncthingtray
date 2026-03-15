#include "./syncthingdata.h"

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
#include "../quick/helpers.h"
#endif

#include "resources/config.h"

namespace QtGui {

SyncthingData::SyncthingData(QObject *parent, bool textOnly, bool clickToConnect)
    : QObject(parent)
    , m_notifier(m_connection)
    , m_statusInfo(textOnly, clickToConnect)
{
    m_connection.setPollingFlags(Data::SyncthingConnection::PollingFlags::MainEvents | Data::SyncthingConnection::PollingFlags::Errors);
}

SyncthingData::~SyncthingData()
{
}

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
SyncthingData *SyncthingData::create(QQmlEngine *qmlEngine, QJSEngine *engine)
{
    return dataObjectFromProperty<SyncthingData>(qmlEngine, engine);
}
#endif

QString SyncthingData::website() const
{
    return QStringLiteral(APP_URL);
}

} // namespace QtGui
