#include "./utils.h"

#include <c++utilities/chrono/datetime.h>

#include <QCoreApplication>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QString>
#include <QUrl>

using namespace ChronoUtilities;

namespace Data {

/*!
 * \brief Returns a string like "2 min 45 s ago" for the specified \a dateTime.
 */
QString agoString(DateTime dateTime)
{
    const TimeSpan delta(DateTime::now() - dateTime);
    if (!delta.isNegative() && static_cast<uint64>(delta.totalTicks()) > (TimeSpan::ticksPerMinute / 4uL)) {
        return QCoreApplication::translate("Data::Utils", "%1 ago")
            .arg(QString::fromUtf8(delta.toString(TimeSpanOutputFormat::WithMeasures, true).data()));
    } else {
        return QCoreApplication::translate("Data::Utils", "right now");
    }
}

/*!
 * \brief Returns whether the host specified by the given \a url is the local machine.
 */
bool isLocal(const QUrl &url)
{
    const QString host(url.host());
    const QHostAddress hostAddress(host);
    return host.compare(QLatin1String("localhost"), Qt::CaseInsensitive) == 0 || hostAddress.isLoopback()
        || QNetworkInterface::allAddresses().contains(hostAddress);
}
}
