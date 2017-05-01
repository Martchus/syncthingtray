#include "./syncthingconnectionsettings.h"

namespace Data {

bool SyncthingConnectionSettings::loadHttpsCert()
{
    expectedSslErrors.clear();
    if (!httpsCertPath.isEmpty()) {
        const QList<QSslCertificate> cert = QSslCertificate::fromPath(httpsCertPath);
        if (cert.isEmpty()) {
            return false;
        }
        expectedSslErrors.reserve(4);
        expectedSslErrors << QSslError(QSslError::UnableToGetLocalIssuerCertificate, cert.at(0));
        expectedSslErrors << QSslError(QSslError::UnableToVerifyFirstCertificate, cert.at(0));
        expectedSslErrors << QSslError(QSslError::SelfSignedCertificate, cert.at(0));
        expectedSslErrors << QSslError(QSslError::HostNameMismatch, cert.at(0));
    }
    return true;
}
}
