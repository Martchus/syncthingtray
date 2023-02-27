#ifndef SYNCTHINGCONNECTIONMOCKHELPERS_H
#define SYNCTHINGCONNECTIONMOCKHELPERS_H

/*!
 * \file syncthingconnectionhelper.h
 * \brief Provides helper for mocking SyncthingConnection.
 * \remarks Only include from syncthingconnection.cpp!
 */

#include <c++utilities/conversion/stringbuilder.h>
#include <c++utilities/io/misc.h>
#include <c++utilities/tests/testutils.h>

#include <QNetworkReply>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace Data {

void setupTestData();

/*!
 * \brief The MockedReply class provides a fake QNetworkReply which will just return data from a specified buffer.
 */
class MockedReply final : public QNetworkReply {
    Q_OBJECT

public:
    ~MockedReply() override;

public Q_SLOTS:
    void abort() override;

public:
    // reimplemented from QNetworkReply
    void close() override;
    qint64 bytesAvailable() const override;
    bool isSequential() const override;
    qint64 size() const override;

    qint64 readData(char *data, qint64 maxlen) override;

    static MockedReply *forRequest(const QString &method, const QString &path, const QUrlQuery &query, bool rest);

protected:
    MockedReply(const std::string &buffer, int delay, QObject *parent = nullptr);

private Q_SLOTS:
    void emitFinished();

private:
    const std::string &m_buffer;
    const char *m_pos;
    qint64 m_bytesLeft;
    static int s_eventIndex;
};
} // namespace Data

#endif // SYNCTHINGCONNECTIONMOCKHELPERS_H
