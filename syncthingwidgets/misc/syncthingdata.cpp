#include "./syncthingdata.h"

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
#include "../quick/helpers.h"
#endif

#include "resources/config.h"

namespace QtGui {

/*!
 * \class SyncthingData
 * \brief The SyncthingData class contains fields for accessing and displaying the most important Syncthing data
 *        and functions.
 * \remarks
 * - An instance of this class is used in most UI components and the app service to avoid specifying these fields
 *   repeatedly in all those places. This class has not been merged with SyncthingModels because the app service
 *   doesn't need the models.
 * - This class is available as singleton in Qml code.
 */

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
