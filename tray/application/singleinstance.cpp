#include "./singleinstance.h"

#include <c++utilities/conversion/binaryconversion.h>
#include <c++utilities/io/ansiescapecodes.h>

#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStringBuilder>

#include <iostream>
#include <memory>

using namespace std;
using namespace CppUtilities;
using namespace CppUtilities::EscapeCodes;

namespace QtGui {

SingleInstance::SingleInstance(int argc, const char *const *argv, QObject *parent)
    : QObject(parent)
    , m_server(nullptr)
{
    const QString appId(QCoreApplication::applicationName() % QStringLiteral(" by ") % QCoreApplication::organizationName());

    // check for previous instance
    QLocalSocket socket;
    socket.connectToServer(appId, QLocalSocket::ReadWrite);
    if (socket.waitForConnected(1000)) {
        cerr << Phrases::Info << "Application already running, sending args to previous instance" << Phrases::EndFlush;
        if (argc >= 0 && argc <= 0xFFFF) {
            char buffer[2];
            BE::getBytes(static_cast<std::uint16_t>(argc), buffer);
            socket.write(buffer, 2);
            *buffer = '\0';
            for (const char *const *end = argv + argc; argv != end; ++argv) {
                socket.write(*argv);
                socket.write(buffer, 1);
            }
        } else {
            cerr << Phrases::Error << "Unable to pass the specified number of arguments" << Phrases::EndFlush;
        }
        socket.flush();
        socket.close();
        exit(0);
    }

    // no previous instance running
    // -> however, previous server instance might not have been cleaned up dute to crash
    QLocalServer::removeServer(appId);
    // -> start server
    m_server = new QLocalServer(this);
    connect(m_server, &QLocalServer::newConnection, this, &SingleInstance::handleNewConnection);
    if (!m_server->listen(appId)) {
        cerr << Phrases::Error << "Unable to launch as single instance application" << Phrases::EndFlush;
    }
}

void SingleInstance::handleNewConnection()
{
    const QLocalSocket *const socket = m_server->nextPendingConnection();
    connect(socket, &QLocalSocket::readChannelFinished, this, &SingleInstance::readArgs);
}

void SingleInstance::readArgs()
{
    auto *const socket = static_cast<QLocalSocket *>(sender());

    // check arg data size
    const auto argDataSize = socket->bytesAvailable();
    if (argDataSize < 2 && argDataSize > (1024 * 1024)) {
        cerr << Phrases::Error << "Another application instance sent invalid argument data." << Phrases::EndFlush;
        return;
    }

    // read arg data
    auto argData = make_unique<char[]>(static_cast<size_t>(argDataSize));
    socket->read(argData.get(), argDataSize);
    socket->close();
    socket->deleteLater();

    // reconstruct argc and argv array
    const auto argc = BE::toUInt16(argData.get());
    vector<const char *> args;
    args.reserve(argc + 1);
    for (const char *argv = argData.get() + 2, *end = argData.get() + argDataSize, *i = argv; i != end && *argv;) {
        if (!*i) {
            args.push_back(argv);
            argv = ++i;
        } else {
            ++i;
        }
    }
    args.push_back(nullptr);

    emit newInstance(static_cast<int>(args.size() - 1), args.data());
}
} // namespace QtGui
