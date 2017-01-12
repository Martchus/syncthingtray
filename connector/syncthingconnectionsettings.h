#ifndef SYNCTHINGCONNECTIONSETTINGS_H
#define SYNCTHINGCONNECTIONSETTINGS_H

#include "./global.h"

#include <QString>
#include <QByteArray>
#include <QSslError>

namespace Data {

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingConnectionSettings {
    QString label;
    QString syncthingUrl;
    bool authEnabled = false;
    QString userName;
    QString password;
    QByteArray apiKey;
    int trafficPollInterval = 2000;
    int devStatsPollInterval = 60000;
    int errorsPollInterval = 30000;
    int reconnectInterval = 0;
    QString httpsCertPath;
    QList<QSslError> expectedSslErrors;
    bool loadHttpsCert();
};

}

#endif // SYNCTHINGCONNECTIONSETTINGS_H
