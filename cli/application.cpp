#include "./application.h"
#include "./helper.h"

#include "../connector/syncthingconfig.h"

#include <c++utilities/application/failure.h>
#include <c++utilities/chrono/timespan.h>
#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/io/ansiescapecodes.h>

#include <qtutilities/misc/conversion.h>

#include <QCoreApplication>
#include <QDir>
#include <QNetworkAccessManager>

#include <functional>
#include <iostream>

using namespace std;
using namespace std::placeholders;
using namespace ApplicationUtilities;
using namespace EscapeCodes;
using namespace ChronoUtilities;
using namespace ConversionUtilities;
using namespace Data;

namespace Cli {

static bool terminated = false;
static int statusCode = 0;

void exitApplication(int statusCode)
{
    statusCode = ::Cli::statusCode;
    terminated = true;
}

inline QString argToQString(const char *arg, int size = -1)
{
#if !defined(PLATFORM_WINDOWS)
    return QString::fromLocal8Bit(arg, size);
#else
    // under Windows args are converted to UTF-8
    return QString::fromUtf8(arg, size);
#endif
}

Application::Application()
    : m_expectedResponse(0)
    , m_preventDisconnect(false)
    , m_callbacksInvoked(false)
{
    // take ownership over the global QNetworkAccessManager
    networkAccessManager().setParent(this);
    exitFunction = &exitApplication;

    // setup argument callbacks
    m_args.status.setCallback(bind(&Application::printStatus, this, _1));
    m_args.log.setCallback(bind(&Application::requestLog, this, _1));
    m_args.stop.setCallback(bind(&Application::requestShutdown, this, _1));
    m_args.restart.setCallback(bind(&Application::requestRestart, this, _1));
    m_args.rescan.setCallback(bind(&Application::requestRescan, this, _1));
    m_args.rescanAll.setCallback(bind(&Application::requestRescanAll, this, _1));
    m_args.pause.setCallback(bind(&Application::requestPauseResume, this, true));
    m_args.resume.setCallback(bind(&Application::requestPauseResume, this, false));
    m_args.pauseAllDevs.setCallback(bind(&Application::requestPauseAllDevs, this, _1));
    m_args.pauseAllDirs.setCallback(bind(&Application::requestPauseAllDirs, this, _1));
    m_args.resumeAllDevs.setCallback(bind(&Application::requestResumeAllDevs, this, _1));
    m_args.resumeAllDirs.setCallback(bind(&Application::requestResumeAllDirs, this, _1));
    m_args.waitForIdle.setCallback(bind(&Application::initWaitForIdle, this, _1));
    m_args.pwd.setCallback(bind(&Application::checkPwdOperationPresent, this, _1));
    m_args.statusPwd.setCallback(bind(&Application::printPwdStatus, this, _1));
    m_args.rescanPwd.setCallback(bind(&Application::requestRescanPwd, this, _1));
    m_args.pausePwd.setCallback(bind(&Application::requestPausePwd, this, _1));
    m_args.resumePwd.setCallback(bind(&Application::requestResumePwd, this, _1));

    // connect signals and slots
    connect(&m_connection, &SyncthingConnection::statusChanged, this, &Application::handleStatusChanged);
    connect(&m_connection, &SyncthingConnection::error, this, &Application::handleError);
}

Application::~Application()
{
}

int Application::exec(int argc, const char *const *argv)
{
    try {
        // parse arguments
        m_args.parser.readArgs(argc, argv);

        // check whether application needs to be terminated due to --bash-completion argument
        if (terminated) {
            return statusCode;
        }

        m_args.parser.checkConstraints();

        // handle help argument
        if (m_args.help.isPresent()) {
            m_args.parser.printHelp(cout);
            return 0;
        }

        // locate and read Syncthing config file
        QString configFile;
        const char *configFileArgValue = m_args.configFile.firstValue();
        if (configFileArgValue) {
            configFile = fromNativeFileName(configFileArgValue);
        } else {
            configFile = SyncthingConfig::locateConfigFile();
        }
        SyncthingConfig config;
        const char *apiKeyArgValue = m_args.apiKey.firstValue();
        if (!config.restore(configFile)) {
            if (configFileArgValue) {
                cerr << "Error: Unable to locate specified Syncthing config file \"" << configFileArgValue << "\"" << endl;
                return -1;
            } else if (!apiKeyArgValue) {
                cerr << "Error: Unable to locate Syncthing config file and no API key specified" << endl;
                return -2;
            }
        }

        // apply settings for connection
        if (const char *urlArgValue = m_args.url.firstValue()) {
            m_settings.syncthingUrl = argToQString(urlArgValue);
        } else if (!config.guiAddress.isEmpty()) {
            m_settings.syncthingUrl = config.syncthingUrl();
        } else {
            m_settings.syncthingUrl = QStringLiteral("http://localhost:8080");
        }
        if (m_args.credentials.isPresent()) {
            m_settings.authEnabled = true;
            m_settings.userName = argToQString(m_args.credentials.values(0)[0]);
            m_settings.password = argToQString(m_args.credentials.values(0)[1]);
        }
        if (apiKeyArgValue) {
            m_settings.apiKey.append(apiKeyArgValue);
        } else {
            m_settings.apiKey.append(config.guiApiKey);
        }
        if (const char *certArgValue = m_args.certificate.firstValue()) {
            m_settings.httpsCertPath = argToQString(certArgValue);
            if (m_settings.httpsCertPath.isEmpty() || !m_settings.loadHttpsCert()) {
                cerr << "Error: Unable to load specified certificate \"" << m_args.certificate.firstValue() << "\"" << endl;
                return -3;
            }
        }

        // finally to request / establish connection
        if (m_args.status.isPresent() || m_args.rescanAll.isPresent() || m_args.pauseAllDirs.isPresent() || m_args.pauseAllDevs.isPresent()
            || m_args.resumeAllDirs.isPresent() || m_args.resumeAllDevs.isPresent() || m_args.pause.isPresent() || m_args.resume.isPresent()
            || m_args.waitForIdle.isPresent() || m_args.pwd.isPresent()) {
            // those arguments rquire establishing a connection first, the actual handler is called by handleStatusChanged() when
            // the connection has been established
            m_connection.reconnect(m_settings);
            cerr << "Connecting to " << m_settings.syncthingUrl.toLocal8Bit().data() << " ...";
            cerr.flush();
        } else {
            // call handler for any other arguments directly
            m_connection.applySettings(m_settings);
            m_args.parser.invokeCallbacks();
        }

        // enter event loop
        return QCoreApplication::exec();

    } catch (const Failure &ex) {
        cerr << "Unable to parse arguments. " << ex.what() << "\nSee --help for available commands." << endl;
        return 1;
    }
}

void Application::handleStatusChanged(SyncthingStatus newStatus)
{
    Q_UNUSED(newStatus)
    if (m_callbacksInvoked) {
        return;
    }
    if (m_connection.isConnected()) {
        eraseLine(cout);
        cout << '\r';
        m_callbacksInvoked = true;
        m_args.parser.invokeCallbacks();
        if (!m_preventDisconnect) {
            m_connection.disconnect();
        }
    }
}

void Application::handleResponse()
{
    if (m_expectedResponse) {
        if (!--m_expectedResponse) {
            QCoreApplication::quit();
        }
    } else {
        cerr << "Error: Unexpected response" << endl;
        QCoreApplication::exit(-4);
    }
}

void Application::handleError(const QString &message)
{
    eraseLine(cout);
    cerr << "\rError: " << message.toLocal8Bit().data() << endl;
    QCoreApplication::exit(-3);
}

void Application::findRelevantDirsAndDevs()
{
    findRelevantDirsAndDevs(OperationType::Status);
}

void Application::requestLog(const ArgumentOccurrence &)
{
    m_connection.requestLog(&Application::printLog);
    cerr << "Request log from " << m_settings.syncthingUrl.toLocal8Bit().data() << " ...";
    cerr.flush();
}

void Application::requestShutdown(const ArgumentOccurrence &)
{
    connect(&m_connection, &SyncthingConnection::shutdownTriggered, &QCoreApplication::quit);
    m_connection.shutdown();
    cerr << "Request shutdown " << m_settings.syncthingUrl.toLocal8Bit().data() << " ...";
    cerr.flush();
}

void Application::requestRestart(const ArgumentOccurrence &)
{
    connect(&m_connection, &SyncthingConnection::restartTriggered, &QCoreApplication::quit);
    m_connection.restart();
    cerr << "Request restart " << m_settings.syncthingUrl.toLocal8Bit().data() << " ...";
    cerr.flush();
}

void Application::requestRescan(const ArgumentOccurrence &occurrence)
{
    m_expectedResponse = occurrence.values.size();
    connect(&m_connection, &SyncthingConnection::rescanTriggered, this, &Application::handleResponse);
    for (const char *value : occurrence.values) {
        cerr << "Request rescanning " << value << " ...\n";
        // split into directory name and relpath
        const char *firstSlash = value;
        for (; *firstSlash && *firstSlash != '/'; ++firstSlash)
            ;
        if (*firstSlash) {
            m_connection.rescan(argToQString(value, static_cast<int>(firstSlash - value)), argToQString(firstSlash + 1));
        } else {
            m_connection.rescan(argToQString(value));
        }
    }
    cerr.flush();
}

void Application::requestRescanAll(const ArgumentOccurrence &)
{
    m_expectedResponse = m_connection.dirInfo().size();
    connect(&m_connection, &SyncthingConnection::rescanTriggered, this, &Application::handleResponse);
    cerr << "Request rescanning all directories ..." << endl;
    m_connection.rescanAllDirs();
}

void Application::requestPauseResume(bool pause)
{
    findRelevantDirsAndDevs(OperationType::PauseResume);
    m_expectedResponse = m_relevantDevs.size();
    if (pause) {
        connect(&m_connection, &SyncthingConnection::devicePauseTriggered, this, &Application::handleResponse);
        connect(&m_connection, &SyncthingConnection::directoryPauseTriggered, this, &Application::handleResponse);
    } else {
        connect(&m_connection, &SyncthingConnection::deviceResumeTriggered, this, &Application::handleResponse);
        connect(&m_connection, &SyncthingConnection::directoryResumeTriggered, this, &Application::handleResponse);
    }
    if (!m_relevantDirs.empty()) {
        QStringList dirIds;
        dirIds.reserve(m_relevantDirs.size());
        for (const SyncthingDir *dir : m_relevantDirs) {
            dirIds << dir->id;
        }
        if (pause) {
            cerr << "Request pausing directories ";
        } else {
            cerr << "Request resuming directories ";
        }
        cerr << dirIds.join(QStringLiteral(", ")).toLocal8Bit().data() << " ...\n";
        if (pause ? m_connection.pauseDirectories(dirIds) : m_connection.resumeDirectories(dirIds)) {
            ++m_expectedResponse;
        }
    }
    for (const SyncthingDev *dev : m_relevantDevs) {
        if (pause) {
            cerr << "Request pausing device ";
        }
        cerr << dev->id.toLocal8Bit().data() << " ...\n";
        pause ? m_connection.pauseDevice(dev->id) : m_connection.resumeDevice(dev->id);
    }
    cerr.flush();
}

void Application::requestPauseAllDevs(const ArgumentOccurrence &)
{
    findRelevantDirsAndDevs(OperationType::PauseResume);
    m_expectedResponse = m_connection.devInfo().size();
    connect(&m_connection, &SyncthingConnection::devicePauseTriggered, this, &Application::handleResponse);
    cerr << "Request pausing all devices ..." << endl;
    m_connection.pauseAllDevs();
}

void Application::requestPauseAllDirs(const ArgumentOccurrence &)
{
    m_expectedResponse = m_connection.dirInfo().size();
    connect(&m_connection, &SyncthingConnection::directoryPauseTriggered, this, &Application::handleResponse);
    cerr << "Request pausing all directories ..." << endl;
    m_connection.pauseAllDirs();
}

void Application::requestResumeAllDevs(const ArgumentOccurrence &)
{
    m_expectedResponse = m_connection.devInfo().size();
    connect(&m_connection, &SyncthingConnection::deviceResumeTriggered, this, &Application::handleResponse);
    cerr << "Request resuming all devices ..." << endl;
    m_connection.resumeAllDevs();
}

void Application::requestResumeAllDirs(const ArgumentOccurrence &)
{
    m_expectedResponse = m_connection.dirInfo().size();
    connect(&m_connection, &SyncthingConnection::deviceResumeTriggered, this, &Application::handleResponse);
    cerr << "Request resuming all directories ..." << endl;
    m_connection.resumeAllDevs();
}

void Application::findRelevantDirsAndDevs(OperationType operationType)
{
    int dummy;

    Argument *dirArg, *devArg;
    switch (operationType) {
    case OperationType::Status:
        dirArg = &m_args.statusDir;
        devArg = &m_args.statusDev;
        break;
    case OperationType::PauseResume:
        dirArg = &m_args.pauseDir;
        devArg = &m_args.pauseDev;
    }

    if (dirArg->isPresent()) {
        m_relevantDirs.reserve(dirArg->occurrences());
        for (size_t i = 0; i != dirArg->occurrences(); ++i) {
            if (const SyncthingDir *dir = m_connection.findDirInfo(argToQString(dirArg->values(i).front()), dummy)) {
                m_relevantDirs.emplace_back(dir);
            } else {
                cerr << "Warning: Specified directory \"" << dirArg->values(i).front() << "\" does not exist and will be ignored" << endl;
            }
        }
    }
    if (devArg->isPresent()) {
        m_relevantDevs.reserve(devArg->occurrences());
        for (size_t i = 0; i != devArg->occurrences(); ++i) {
            const SyncthingDev *dev = m_connection.findDevInfo(argToQString(devArg->values(i).front()), dummy);
            if (!dev) {
                dev = m_connection.findDevInfoByName(argToQString(devArg->values(i).front()), dummy);
            }
            if (dev) {
                m_relevantDevs.emplace_back(dev);
            } else {
                cerr << "Warning: Specified device \"" << devArg->values(i).front() << "\" does not exist and will be ignored" << endl;
            }
        }
    }
    if (operationType == OperationType::Status) {
        // when displaying status information and no dirs/devs have been specified, just print information for all
        if (m_relevantDirs.empty() && m_relevantDevs.empty()) {
            m_relevantDirs.reserve(m_connection.dirInfo().size());
            for (const SyncthingDir &dir : m_connection.dirInfo()) {
                m_relevantDirs.emplace_back(&dir);
            }
            m_relevantDevs.reserve(m_connection.devInfo().size());
            for (const SyncthingDev &dev : m_connection.devInfo()) {
                m_relevantDevs.emplace_back(&dev);
            }
        }
    }
}

bool Application::findPwd()
{
    const QString pwd(QDir::currentPath());
    for (const SyncthingDir &dir : m_connection.dirInfo()) {
        if (pwd == dir.pathWithoutTrailingSlash()) {
            m_pwd = &dir;
            return true;
        } else if (pwd.startsWith(dir.path)) {
            m_pwd = &dir;
            m_relativePath = pwd.mid(dir.path.size());
            return true;
        }
    }
    cerr << "Error: The current working directory \"" << pwd.toLocal8Bit().data() << "\" is not (part of) a Syncthing directory." << endl;
    QCoreApplication::exit(2);
    return false;
}

void Application::printDir(const SyncthingDir *dir)
{
    cout << " - ";
    setStyle(cout, TextAttribute::Bold);
    cout << dir->id.toLocal8Bit().data() << '\n';
    setStyle(cout);
    printProperty("Label", dir->label);
    printProperty("Path", dir->path);
    printProperty("Status", dir->statusString());
    printProperty("Last scan time", dir->lastScanTime);
    printProperty("Last file time", dir->lastFileTime);
    printProperty("Last file name", dir->lastFileName);
    printProperty("Download progress", dir->downloadLabel);
    printProperty("Devices", dir->devices);
    printProperty("Read-only", dir->readOnly);
    printProperty("Ignore permissions", dir->ignorePermissions);
    printProperty("Auto-normalize", dir->autoNormalize);
    printProperty("Rescan interval", TimeSpan::fromSeconds(dir->rescanInterval));
    printProperty("Min. free disk percentage", dir->minDiskFreePercentage);
    if (!dir->errors.empty()) {
        cout << "   Errors\n";
        for (const SyncthingDirError &error : dir->errors) {
            printProperty(" - Message", error.message);
            printProperty("   File", error.path);
        }
    }
    cout << '\n';
}

void Application::printDev(const SyncthingDev *dev)
{
    cout << " - ";
    setStyle(cout, TextAttribute::Bold);
    cout << dev->name.toLocal8Bit().data() << '\n';
    setStyle(cout);
    printProperty("ID", dev->id);
    printProperty("Status", dev->statusString());
    printProperty("Addresses", dev->addresses);
    printProperty("Compression", dev->compression);
    printProperty("Cert name", dev->certName);
    printProperty("Connection address", dev->connectionAddress);
    printProperty("Connection type", dev->connectionType);
    printProperty("Client version", dev->clientVersion);
    printProperty("Last seen", dev->lastSeen);
    if (dev->totalIncomingTraffic > 0) {
        printProperty("Incoming traffic", dataSizeToString(static_cast<uint64>(dev->totalIncomingTraffic)).data());
    }
    if (dev->totalOutgoingTraffic > 0) {
        printProperty("Outgoing traffic", dataSizeToString(static_cast<uint64>(dev->totalOutgoingTraffic)).data());
    }
    cout << '\n';
}

void Application::printStatus(const ArgumentOccurrence &)
{
    findRelevantDirsAndDevs();

    // display dirs
    if (!m_relevantDirs.empty()) {
        setStyle(cout, TextAttribute::Bold);
        cout << "Directories\n";
        setStyle(cout);
        for_each(m_relevantDirs.cbegin(), m_relevantDirs.cend(), &Application::printDir);
    }

    // display devs
    if (!m_relevantDevs.empty()) {
        setStyle(cout, TextAttribute::Bold);
        cout << "Devices\n";
        setStyle(cout);
        for_each(m_relevantDevs.cbegin(), m_relevantDevs.cend(), &Application::printDev);
    }

    cout.flush();
    QCoreApplication::exit();
}

void Application::printLog(const std::vector<SyncthingLogEntry> &logEntries)
{
    eraseLine(cout);
    cout << '\r';

    for (const SyncthingLogEntry &entry : logEntries) {
        cout << DateTime::fromIsoStringLocal(entry.when.toLocal8Bit().data()).toString(DateTimeOutputFormat::DateAndTime, true).data() << ':' << ' '
             << entry.message.toLocal8Bit().data() << '\n';
    }
    cout.flush();
    QCoreApplication::exit();
}

void Application::initWaitForIdle(const ArgumentOccurrence &)
{
    m_preventDisconnect = true;

    findRelevantDirsAndDevs();

    // might idle already
    waitForIdle();

    // currently not idling
    // -> relevant dirs/devs might be invalidated so findRelevantDirsAndDevs() must invoked again
    connect(&m_connection, &SyncthingConnection::newDirs, this, static_cast<void (Application::*)(void)>(&Application::findRelevantDirsAndDevs));
    connect(&m_connection, &SyncthingConnection::newDevices, this, static_cast<void (Application::*)(void)>(&Application::findRelevantDirsAndDevs));
    // -> check for idle again when dir/dev status changed
    connect(&m_connection, &SyncthingConnection::dirStatusChanged, this, &Application::waitForIdle);
    connect(&m_connection, &SyncthingConnection::devStatusChanged, this, &Application::waitForIdle);
}

void Application::waitForIdle()
{
    for (const SyncthingDir *dir : m_relevantDirs) {
        switch (dir->status) {
        case SyncthingDirStatus::Unknown:
        case SyncthingDirStatus::Idle:
        case SyncthingDirStatus::Unshared:
            break;
        default:
            return;
        }
    }
    for (const SyncthingDev *dev : m_relevantDevs) {
        switch (dev->status) {
        case SyncthingDevStatus::Unknown:
        case SyncthingDevStatus::Disconnected:
        case SyncthingDevStatus::OwnDevice:
        case SyncthingDevStatus::Idle:
            break;
        default:
            return;
        }
    }
    QCoreApplication::exit();
}

void Application::checkPwdOperationPresent(const ArgumentOccurrence &occurrence)
{
    // FIXME: implement requiring at least one operation and default operation in argument parser
    for (const Argument *pwdOperationArg : m_args.pwd.subArguments()) {
        if (pwdOperationArg->denotesOperation() && pwdOperationArg->isPresent()) {
            return;
        }
    }
    // print status when no operation specified
    printPwdStatus(occurrence);
}

void Application::printPwdStatus(const ArgumentOccurrence &)
{
    if (!findPwd()) {
        return;
    }
    printDir(m_pwd);
    QCoreApplication::quit();
}

void Application::requestRescanPwd(const ArgumentOccurrence &)
{
    if (!findPwd()) {
        return;
    }
    if (m_relativePath.isEmpty()) {
        cerr << "Request rescanning directory \"" << m_pwd->path.toLocal8Bit().data() << "\" ..." << endl;
    } else {
        cerr << "Request rescanning item \"" << m_relativePath.toLocal8Bit().data() << "\" in directory \"" << m_pwd->path.toLocal8Bit().data()
             << "\" ..." << endl;
    }
    m_connection.rescan(m_pwd->id, m_relativePath);
    connect(&m_connection, &SyncthingConnection::rescanTriggered, this, &Application::handleResponse);
    m_expectedResponse = 1;
}

void Application::requestPausePwd(const ArgumentOccurrence &)
{
    if (!findPwd()) {
        return;
    }
    if (m_connection.pauseDirectories(QStringList(m_pwd->id))) {
        cerr << "Request pausing directory \"" << m_pwd->path.toLocal8Bit().data() << "\" ..." << endl;
        connect(&m_connection, &SyncthingConnection::directoryPauseTriggered, this, &Application::handleResponse);
        m_preventDisconnect = true;
        m_expectedResponse = 1;
    } else {
        cerr << "Directory \"" << m_pwd->path.toLocal8Bit().data() << " already paused" << endl;
        QCoreApplication::quit();
    }
}

void Application::requestResumePwd(const ArgumentOccurrence &)
{
    if (!findPwd()) {
        return;
    }
    if (m_connection.resumeDirectories(QStringList(m_pwd->id))) {
        cerr << "Request resuming directory \"" << m_pwd->path.toLocal8Bit().data() << "\" ..." << endl;
        connect(&m_connection, &SyncthingConnection::directoryResumeTriggered, this, &Application::handleResponse);
        m_preventDisconnect = true;
        m_expectedResponse = 1;
        return;
    } else {
        cerr << "Directory \"" << m_pwd->path.toLocal8Bit().data() << " not paused" << endl;
        QCoreApplication::quit();
    }
}

} // namespace Cli
