#include "./syncthingconnectionsettings.h"

namespace Data {

bool SyncthingConnectionSettings::loadHttpsCert()
{
    if(!httpsCertPath.isEmpty()) {
        const QList<QSslCertificate> cert = QSslCertificate::fromPath(httpsCertPath);
        if(cert.isEmpty()) {
            return false;
        }
        expectedSslErrors.clear();
        expectedSslErrors.reserve(4);
        expectedSslErrors << QSslError(QSslError::UnableToGetLocalIssuerCertificate, cert.at(0));
        expectedSslErrors << QSslError(QSslError::UnableToVerifyFirstCertificate, cert.at(0));
        expectedSslErrors << QSslError(QSslError::SelfSignedCertificate, cert.at(0));
        expectedSslErrors << QSslError(QSslError::HostNameMismatch, cert.at(0));
    }
    return true;
}

}
