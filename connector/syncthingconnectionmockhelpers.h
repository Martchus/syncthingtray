#ifndef SYNCTHINGCONNECTIONMOCKHELPERS_H
#define SYNCTHINGCONNECTIONMOCKHELPERS_H

/*!
 * \file syncthingconnectionhelper.h
 * \brief Provides helper for mocking SyncthingConnection.
 * \remarks Only include from syncthingconnection.cpp!
 */

#include <c++utilities/conversion/stringbuilder.h>
#include <c++utilities/io/catchiofailure.h>
#include <c++utilities/io/misc.h>
#include <c++utilities/tests/testutils.h>

#include <QNetworkReply>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

using namespace std;
using namespace IoUtilities;
using namespace ConversionUtilities;

namespace Data {

void setupTestData();

/*!
 * \brief The MockedReply class provides a fake QNetworkReply which will just return data from a specified buffer.
 */
class MockedReply : public QNetworkReply {
    Q_OBJECT

public:
    ~MockedReply();

public Q_SLOTS:
    void abort() Q_DECL_OVERRIDE;

public:
    // reimplemented from QNetworkReply
    void close() Q_DECL_OVERRIDE;
    qint64 bytesAvailable() const Q_DECL_OVERRIDE;
    bool isSequential() const Q_DECL_OVERRIDE;
    qint64 size() const Q_DECL_OVERRIDE;

    qint64 readData(char *data, qint64 maxlen) Q_DECL_OVERRIDE;

    static MockedReply *forRequest(const QString &method, const QString &path, const QUrlQuery &query, bool rest);

protected:
    MockedReply(const string &buffer, int delay, QObject *parent = nullptr);

private Q_SLOTS:
    void emitFinished();

private:
    const string &m_buffer;
    const char *m_pos;
    qint64 m_bytesLeft;
};
} // namespace Data

#endif // SYNCTHINGCONNECTIONMOCKHELPERS_H
