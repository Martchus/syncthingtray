#include "./singleinstance.h"

#include <c++utilities/conversion/binaryconversion.h>
#include <c++utilities/io/ansiescapecodes.h>

#include <QCoreApplication>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStringBuilder>
#include <QThread>

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

SingleInstance::SingleInstance(int argc, const char *const *argv, bool skipSingleInstanceBehavior, bool skipPassing, QObject *parent)
    : QObject(parent)
    , m_server(nullptr)
{
    // just do nothing if supposed to skip single instance behavior
    if (skipSingleInstanceBehavior) {
        return;
    }

    // check for running instance; if there is one pass parameters and exit
    static const auto appId = applicationId();
    if (!skipPassing && passArgsToRunningInstance(argc, argv, appId)) {
        std::exit(EXIT_SUCCESS);
    }

    // create local server; at this point no previous instance is running anymore
    // -> cleanup possible leftover (previous instance might have crashed)
    QLocalServer::removeServer(appId);
    // -> setup server
    m_server = new QLocalServer(this);
    connect(m_server, &QLocalServer::newConnection, this, &SingleInstance::handleNewConnection);
    if (!m_server->listen(appId)) {
        cerr << Phrases::Error << "Unable to launch as single instance application as " << appId.toStdString() << Phrases::EndFlush;
    } else {
        cerr << Phrases::Info << "Single instance application ID: " << appId.toStdString() << Phrases::EndFlush;
    }
}

const QString &SingleInstance::applicationId()
{
    static const auto id = QString(QCoreApplication::applicationName() % QChar('-') % QCoreApplication::organizationName() % QChar('-') %
#ifdef Q_OS_WINDOWS
        getCurrentProcessSIDAsString()
#else
        QString::number(getuid())
#endif
    );
    return id;
}

bool SingleInstance::passArgsToRunningInstance(int argc, const char *const *argv, const QString &appId, bool waitUntilGone)
{
    if (argc < 0 || argc > 0xFFFF) {
        cerr << Phrases::Error << "Unable to pass the specified number of arguments" << Phrases::EndFlush;
        return false;
    }
    auto socket = QLocalSocket();
    socket.connectToServer(appId, QLocalSocket::ReadWrite);
    const auto fullServerName = socket.fullServerName();
    if (!socket.waitForConnected(1000)) {
        return false;
    }
    cerr << Phrases::Info << "Application already running, sending args to previous instance" << Phrases::EndFlush;
    char buffer[2];
    BE::getBytes(static_cast<std::uint16_t>(argc), buffer);
    auto error = socket.write(buffer, 2) < 0;
    *buffer = '\0';
    for (const char *const *end = argv + argc; argv != end && !error; ++argv) {
        error = socket.write(*argv) < 0 || socket.write(buffer, 1) < 0;
    }
    error = error || !socket.flush();
    socket.disconnectFromServer();
    if (socket.state() != QLocalSocket::UnconnectedState) {
        error = !socket.waitForDisconnected(1000) || error;
    }
    if (error) {
        cerr << Phrases::Error << "Unable to pass args to previous instance: " << socket.errorString().toStdString() << Phrases::EndFlush;
    }
    if (waitUntilGone) {
        cerr << Phrases::Info << "Waiting for previous instance to shutdown" << Phrases::EndFlush;
        while (QFile::exists(fullServerName)) {
            QThread::msleep(500);
        }
    }
    return !error;
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
    auto args = vector<const char *>();
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
