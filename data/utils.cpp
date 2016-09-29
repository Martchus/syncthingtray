#include "./utils.h"

#include <c++utilities/chrono/datetime.h>

#include <QString>
#include <QCoreApplication>

using namespace ChronoUtilities;

namespace Data {

QString agoString(DateTime dateTime)
{
    const TimeSpan delta(DateTime::now() - dateTime);
    if(!delta.isNegative() && static_cast<uint64>(delta.totalTicks()) > (TimeSpan::ticksPerMinute / 4uL)) {
        return QCoreApplication::translate("Data::Utils", "%1 ago").arg(QString::fromLatin1(delta.toString(TimeSpanOutputFormat::WithMeasures, true).data()));
    } else {
        return QCoreApplication::translate("Data::Utils", "right now");
    }
}

}
