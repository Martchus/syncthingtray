#ifndef SYNCTHINGCONNECTIONSETTINGS_H
#define SYNCTHINGCONNECTIONSETTINGS_H

#include "./global.h"

#include <QByteArray>
#include <QList>
#include <QSslError>
#include <QString>

namespace Data {

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingConnectionSettings {
    QString label;
    QString syncthingUrl;
    bool authEnabled = false;
    QString userName;
    QString password;
    QByteArray apiKey;
    int trafficPollInterval = defaultTrafficPollInterval;
    int devStatsPollInterval = defaultDevStatusPollInterval;
    int errorsPollInterval = defaultErrorsPollInterval;
    int reconnectInterval = defaultReconnectInterval;
    QString httpsCertPath;
    QList<QSslError> expectedSslErrors;
    bool autoConnect = false;
    bool loadHttpsCert();

    static constexpr int defaultTrafficPollInterval = 5000;
    static constexpr int defaultDevStatusPollInterval = 60000;
    static constexpr int defaultErrorsPollInterval = 30000;
    static constexpr int defaultReconnectInterval = 0;
};
} // namespace Data

#endif // SYNCTHINGCONNECTIONSETTINGS_H
