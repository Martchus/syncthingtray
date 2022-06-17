#include "./singleinstance.h"

#include <c++utilities/conversion/binaryconversion.h>
#include <c++utilities/io/ansiescapecodes.h>

#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStringBuilder>

#include <iostream>
#include <memory>

#ifdef Q_OS_WINDOWS
#include <windows.h>
// needs to be included after windows.h
#include <sddl.h>
#else
#include <unistd.h>
#endif

using namespace std;
using namespace CppUtilities;
using namespace CppUtilities::EscapeCodes;

namespace QtGui {

#ifdef Q_OS_WINDOWS
static QString getCurrentProcessSIDAsString()
{
    auto res = QString();
    auto processToken = HANDLE();
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &processToken)) {
        std::cerr << Phrases::Error << "Unable to determine current user: OpenProcessToken failed with " << GetLastError() << Phrases::EndFlush;
        return res;
    }

    auto bufferSize = DWORD();
    if (!GetTokenInformation(processToken, TokenUser, nullptr, 0, &bufferSize) && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
        std::cerr << Phrases::Error << "Unable to determine current user: GetTokenInformation failed with " << GetLastError() << Phrases::EndFlush;
        CloseHandle(processToken);
        return res;
    }
    auto buffer = std::vector<BYTE>();
    buffer.resize(bufferSize);

    auto userToken = reinterpret_cast<PTOKEN_USER>(buffer.data());
    if (!GetTokenInformation(processToken, TokenUser, userToken, bufferSize, &bufferSize)) {
        std::cerr << Phrases::Error << "Unable to determine current user: GetTokenInformation failed with " << GetLastError() << Phrases::EndFlush;
        CloseHandle(processToken);
        return res;
    }

    auto stringSid = LPWSTR();
    if (!ConvertSidToStringSidW(userToken->User.Sid, &stringSid)) {
        std::cerr << Phrases::Error << "Unable to determine current user: ConvertSidToStringSid failed with " << GetLastError() << Phrases::EndFlush;
        CloseHandle(processToken);
        return res;
    }
    res = QString::fromWCharArray(stringSid);
    LocalFree(stringSid);
    CloseHandle(processToken);
    return res;
}
#endif

SingleInstance::SingleInstance(int argc, const char *const *argv, bool newInstance, QObject *parent)
    : QObject(parent)
    , m_server(nullptr)
{
    if (newInstance) {
        return;
    }

    // check for running instance
    static const auto appId = QString(QCoreApplication::applicationName() % QChar('-') % QCoreApplication::organizationName() % QChar('-') %
#ifdef Q_OS_WINDOWS
        getCurrentProcessSIDAsString()
#else
        QString::number(getuid())
#endif
    );
    passArgsToRunningInstance(argc, argv, appId);

    // no previous instance running
    // -> however, previous server instance might not have been cleaned up dute to crash
    QLocalServer::removeServer(appId);
    // -> start server
    m_server = new QLocalServer(this);
    connect(m_server, &QLocalServer::newConnection, this, &SingleInstance::handleNewConnection);
    if (!m_server->listen(appId)) {
        cerr << Phrases::Error << "Unable to launch as single instance application as " << appId.toStdString() << Phrases::EndFlush;
    }
}

void SingleInstance::passArgsToRunningInstance(int argc, const char *const *argv, const QString &appId)
{
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
