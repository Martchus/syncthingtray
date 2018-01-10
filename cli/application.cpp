#include "./application.h"
#include "./helper.h"

#include "../connector/syncthingconfig.h"
#include "../connector/utils.h"

// use header-only functions waitForSignals() and signalInfo() from test utilities; disable assertions via macro
#define SYNCTHINGTESTHELPER_FOR_CLI
#include "../testhelper/helper.h"

#include <c++utilities/application/failure.h>
#include <c++utilities/chrono/timespan.h>
#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/io/ansiescapecodes.h>

#include <qtutilities/misc/conversion.h>

#include <QCoreApplication>
#include <QDir>
#include <QNetworkAccessManager>
#include <QStringBuilder>
#include <QTimer>

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
    , m_idleDuration(0)
    , m_idleTimeout(0)
    , m_argsRead(false)
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
    m_args.waitForIdle.setCallback(bind(&Application::waitForIdle, this, _1));
    m_args.pwd.setCallback(bind(&Application::checkPwdOperationPresent, this, _1));
    m_args.statusPwd.setCallback(bind(&Application::printPwdStatus, this, _1));
    m_args.rescanPwd.setCallback(bind(&Application::requestRescanPwd, this, _1));
    m_args.pausePwd.setCallback(bind(&Application::requestPausePwd, this, _1));
    m_args.resumePwd.setCallback(bind(&Application::requestResumePwd, this, _1));
    m_args.pauseDir.setCallback(bind(&Application::initDirCompletion, this, ref(m_args.pauseDir), _1));
    m_args.statusDir.setCallback(bind(&Application::initDirCompletion, this, ref(m_args.statusDir), _1));
    m_args.pauseDev.setCallback(bind(&Application::initDevCompletion, this, ref(m_args.pauseDev), _1));
    m_args.statusDev.setCallback(bind(&Application::initDevCompletion, this, ref(m_args.statusDev), _1));

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
        m_argsRead = true;

    } catch (const Failure &failure) {
        cerr << failure;
        return 1;
    }

    // handle help argument
    if (m_args.help.isPresent()) {
        m_args.parser.printHelp(cout);
        return 0;
    }

    // load configuration
    if (const int res = loadConfig()) {
        return res;
    }

    // finally do the request or establish connection
    if (m_args.status.isPresent() || m_args.rescan.isPresent() || m_args.rescanAll.isPresent() || m_args.pauseAllDirs.isPresent()
        || m_args.pauseAllDevs.isPresent() || m_args.resumeAllDirs.isPresent() || m_args.resumeAllDevs.isPresent() || m_args.pause.isPresent()
        || m_args.resume.isPresent() || m_args.waitForIdle.isPresent() || m_args.pwd.isPresent()) {
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
}

int assignIntegerFromArg(const Argument &arg, int &integer)
{
    if (arg.isPresent()) {
        try {
            integer = stringToNumber<int>(arg.firstValue());
            if (integer < 0) {
                throw ConversionException();
            }
        } catch (const ConversionException &) {
            cerr << Phrases::Error << "The specified number of milliseconds \"" << arg.firstValue() << "\" is no unsigned integer." << Phrases::End
                 << flush;
            return -4;
        }
    }
    return 0;
}

int Application::loadConfig()
{
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
            cerr << Phrases::Error << "Unable to locate specified Syncthing config file \"" << configFileArgValue << "\"" << Phrases::End << flush;
            return -1;
        } else if (!apiKeyArgValue) {
            cerr << Phrases::Error << "Unable to locate Syncthing config file and no API key specified" << Phrases::End << flush;
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
            cerr << Phrases::Error << "Unable to load specified certificate \"" << m_args.certificate.firstValue() << '\"' << Phrases::End << flush;
            return -3;
        }
    }

    // read idle duration and timeout
    if (const int res = assignIntegerFromArg(m_args.atLeast, m_idleDuration)) {
        return res;
    }
    if (const int res = assignIntegerFromArg(m_args.timeout, m_idleTimeout)) {
        return res;
    }

    return 0;
}

void Application::waitForConnected(int timeout)
{
    using namespace TestUtilities;
    bool isConnected = m_connection.isConnected();
    const function<void(SyncthingStatus)> checkStatus([this, &isConnected](SyncthingStatus) { isConnected = m_connection.isConnected(); });
    waitForSignals(bind(static_cast<void (SyncthingConnection::*)(SyncthingConnectionSettings &)>(&SyncthingConnection::reconnect), ref(m_connection),
                       ref(m_settings)),
        timeout, signalInfo(&m_connection, &SyncthingConnection::statusChanged, checkStatus, &isConnected));
}

void Application::handleStatusChanged(SyncthingStatus newStatus)
{
    Q_UNUSED(newStatus)
    // skip when callbacks have already been invoked, when doing shell completion or not connected yet
    if (!m_argsRead || m_callbacksInvoked || !m_connection.isConnected()) {
        return;
    }
    // erase current line
    eraseLine(cout);
    cout << '\r';
    // invoke callbacks
    m_callbacksInvoked = true;
    m_args.parser.invokeCallbacks();
    // disconnect, except when m_preventDisconnect has been set in callbacks
    if (!m_preventDisconnect) {
        m_connection.disconnect();
    }
}

void Application::handleResponse()
{
    if (m_expectedResponse) {
        if (!--m_expectedResponse) {
            QCoreApplication::quit();
        }
    } else {
        cerr << Phrases::Error << "Unexpected response" << Phrases::End << flush;
        QCoreApplication::exit(-4);
    }
}

void Application::handleError(
    const QString &message, SyncthingErrorCategory category, int networkError, const QNetworkRequest &request, const QByteArray &response)
{
    VAR_UNUSED(category)
    VAR_UNUSED(networkError)

    // skip error handling for shell completion
    if (!m_argsRead) {
        return;
    }

    // print error message and relevant request and response if present
    eraseLine(cout);
    cerr << '\n' << '\r' << Phrases::Error;
    cerr << message.toLocal8Bit().data() << Phrases::End;
    const QUrl url(request.url());
    if (!url.isEmpty()) {
        cerr << "\nRequest: " << url.toString(QUrl::PrettyDecoded).toLocal8Bit().data() << '\n';
    }
    if (!response.isEmpty()) {
        cerr << "\nResponse:\n" << response.data() << '\n';
    }
    cerr << flush;
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
    if (!m_argsRead) {
        initDirCompletion(m_args.rescan, occurrence);
        return;
    }

    m_expectedResponse = 0;
    connect(&m_connection, &SyncthingConnection::rescanTriggered, this, &Application::handleResponse);
    for (const char *value : occurrence.values) {
        const QString dirIdentifier(argToQString(value));
        const RelevantDir relevantDir(findDirectory(dirIdentifier));
        if (!relevantDir.dirObj) {
            continue;
        }
        relevantDir.notifyAboutRescan();
        m_connection.rescan(relevantDir.dirObj->id, relevantDir.subDir);
        ++m_expectedResponse;
    }
    if (!m_expectedResponse) {
        cerr << Phrases::Error << "No (valid) directories specified." << Phrases::End << flush;
        exit(1);
    }
    cerr << flush;
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
    m_expectedResponse = 0;
    if (pause) {
        connect(&m_connection, &SyncthingConnection::devicePauseTriggered, this, &Application::handleResponse);
        connect(&m_connection, &SyncthingConnection::directoryPauseTriggered, this, &Application::handleResponse);
    } else {
        connect(&m_connection, &SyncthingConnection::deviceResumeTriggered, this, &Application::handleResponse);
        connect(&m_connection, &SyncthingConnection::directoryResumeTriggered, this, &Application::handleResponse);
    }
    if (m_relevantDirs.empty() && m_relevantDevs.empty()) {
        cerr << Phrases::Error << "No directories or devices specified." << Phrases::End << flush;
        exit(1);
    }
    if (!m_relevantDirs.empty()) {
        QStringList dirIds;
        dirIds.reserve(m_relevantDirs.size());
        for (const RelevantDir &dir : m_relevantDirs) {
            dirIds << dir.dirObj->id;
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
    if (!m_relevantDevs.empty()) {
        QStringList devIds;
        devIds.reserve(m_relevantDirs.size());
        for (const SyncthingDev *dev : m_relevantDevs) {
            devIds << dev->id;
        }
        if (pause) {
            cerr << "Request pausing devices ";
        } else {
            cerr << "Request resuming devices ";
        }
        cerr << devIds.join(QStringLiteral(", ")).toLocal8Bit().data() << " ...\n";
        if (pause ? m_connection.pauseDirectories(devIds) : m_connection.resumeDirectories(devIds)) {
            ++m_expectedResponse;
        }
    }
    if (!m_expectedResponse) {
        cerr << Phrases::Warning << "No directories or devices altered." << Phrases::End << flush;
        exit(0);
    }
    cerr << flush;
}

void Application::requestPauseAllDevs(const ArgumentOccurrence &)
{
    findRelevantDirsAndDevs(OperationType::PauseResume);
    connect(&m_connection, &SyncthingConnection::devicePauseTriggered, this, &Application::handleResponse);
    if (!m_connection.pauseAllDevs()) {
        cerr << Phrases::Warning << "No devices to be paused." << Phrases::End << flush;
        exit(0);
    }
    cerr << "Request pausing all devices ..." << endl;
    m_expectedResponse = 1;
}

void Application::requestPauseAllDirs(const ArgumentOccurrence &)
{
    connect(&m_connection, &SyncthingConnection::directoryPauseTriggered, this, &Application::handleResponse);
    if (!m_connection.pauseAllDirs()) {
        cerr << Phrases::Warning << "No directories to be paused." << Phrases::End << flush;
        exit(0);
    }
    cerr << "Request pausing all directories ..." << endl;
    m_expectedResponse = 1;
}

void Application::requestResumeAllDevs(const ArgumentOccurrence &)
{
    connect(&m_connection, &SyncthingConnection::deviceResumeTriggered, this, &Application::handleResponse);
    if (!m_connection.resumeAllDevs()) {
        cerr << Phrases::Warning << "No devices to be resumed." << Phrases::End << flush;
        exit(0);
    }
    cerr << "Request resuming all devices ..." << endl;
    m_expectedResponse = 1;
}

void Application::requestResumeAllDirs(const ArgumentOccurrence &)
{
    connect(&m_connection, &SyncthingConnection::deviceResumeTriggered, this, &Application::handleResponse);
    if (!m_connection.resumeAllDirs()) {
        cerr << Phrases::Warning << "No directories to be resumed." << Phrases::End << flush;
        exit(0);
    }
    cerr << "Request resuming all directories ..." << endl;
    m_expectedResponse = 1;
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
            const QString dirIdentifier(argToQString(dirArg->values(i).front()));
            const RelevantDir relevantDir(findDirectory(dirIdentifier));
            if (relevantDir.dirObj) {
                m_relevantDirs.emplace_back(move(relevantDir));
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
                cerr << Phrases::Warning << "Specified device \"" << devArg->values(i).front() << "\" does not exist and will be ignored."
                     << Phrases::End;
            }
        }
    }
    if (operationType == OperationType::Status) {
        // when displaying status information and no dirs/devs have been specified, just print information for all
        if (m_relevantDirs.empty() && m_relevantDevs.empty()) {
            m_relevantDirs.reserve(m_connection.dirInfo().size());
            for (const SyncthingDir &dir : m_connection.dirInfo()) {
                m_relevantDirs.emplace_back(&dir, QString());
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

    // find directory for working directory
    int dummy;
    m_pwd.dirObj = m_connection.findDirInfoByPath(pwd, m_pwd.subDir, dummy);
    if (m_pwd) {
        return true;
    }

    // handle error
    cerr << Phrases::Error << "The current working directory \"" << pwd.toLocal8Bit().data() << "\" is not (part of) a Syncthing directory.";
    cerr << Phrases::End << flush;
    QCoreApplication::exit(2);
    return false;
}

void Application::printDir(const RelevantDir &relevantDir)
{
    const SyncthingDir *const dir = relevantDir.dirObj;
    cout << " - ";
    setStyle(cout, TextAttribute::Bold);
    cout << dir->id.toLocal8Bit().data() << '\n';
    setStyle(cout);
    printProperty("Label", dir->label);
    printProperty("Path", dir->path);
    printProperty("Status", dir->statusString());
    printProperty("Global", directoryStatusString(dir->globalFiles, dir->globalDirs, dir->globalBytes), nullptr, 6);
    printProperty("Local", directoryStatusString(dir->localFiles, dir->localDirs, dir->localBytes), nullptr, 6);
    printProperty("Last scan time", dir->lastScanTime);
    printProperty("Last file time", dir->lastFileTime);
    printProperty("Last file name", dir->lastFileName);
    printProperty("Download progress", dir->downloadLabel);
    printProperty("Shared with", dir->deviceNames.isEmpty() ? dir->deviceIds : dir->deviceNames);
    printProperty("Read-only", dir->readOnly);
    printProperty("Ignore permissions", dir->ignorePermissions);
    printProperty("Auto-normalize", dir->autoNormalize);
    printProperty("Rescan interval", TimeSpan::fromSeconds(dir->rescanInterval));
    printProperty("Min. free disk percentage", dir->minDiskFreePercentage);
    printProperty("Error", dir->globalError);
    if (!dir->itemErrors.empty()) {
        cout << "   Failed items\n";
        for (const SyncthingItemError &error : dir->itemErrors) {
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

void Application::waitForIdle(const ArgumentOccurrence &)
{
    m_preventDisconnect = true;

    // setup timer
    QTimer idleTime;
    idleTime.setSingleShot(true);
    idleTime.setInterval(m_idleDuration);

    // define variable which is set to true if handleTimeout to indicate the idle state has persisted long enough
    bool isLongEnoughIdle = false;

    // define handler for timer timeout
    function<void(void)> handleTimeout([this, &isLongEnoughIdle] {
        if (checkWhetherIdle()) {
            isLongEnoughIdle = true;
        }
    });

    // define handler for dirStatusChanged/devStatusChanged
    function<void(void)> handleStatusChange([this, &idleTime] {
        if (!checkWhetherIdle()) {
            idleTime.stop();
            return;
        }
        if (!idleTime.isActive()) {
            idleTime.start();
        }
    });

    // define handler for newDirs/newDevices to call findRelevantDirsAndDevs() in that case
    function<void(void)> handleNewDirsOrDevs([this, &handleStatusChange] {
        findRelevantDirsAndDevs();
        handleStatusChange();
    });

    // invoke handler manually because Syncthing could already be idling
    handleNewDirsOrDevs();

    using namespace TestUtilities;
    waitForSignals(&noop, m_idleTimeout, signalInfo(&m_connection, &SyncthingConnection::dirStatusChanged, handleStatusChange, &isLongEnoughIdle),
        signalInfo(&m_connection, &SyncthingConnection::devStatusChanged, handleStatusChange, &isLongEnoughIdle),
        signalInfo(&m_connection, &SyncthingConnection::newDirs, handleNewDirsOrDevs, &isLongEnoughIdle),
        signalInfo(&m_connection, &SyncthingConnection::newDevices, handleNewDirsOrDevs, &isLongEnoughIdle),
        signalInfo(&idleTime, &QTimer::timeout, handleTimeout, &isLongEnoughIdle));

    if (!isLongEnoughIdle) {
        cerr << Phrases::Warning << "Exiting after timeout" << Phrases::End << flush;
    }
    QCoreApplication::exit(isLongEnoughIdle ? 0 : 1);
}

bool Application::checkWhetherIdle() const
{
    for (const RelevantDir &dir : m_relevantDirs) {
        switch (dir.dirObj->status) {
        case SyncthingDirStatus::Unknown:
        case SyncthingDirStatus::Idle:
        case SyncthingDirStatus::Unshared:
            break;
        default:
            return false;
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
            return false;
        }
    }
    return true;
}

void Application::checkPwdOperationPresent(const ArgumentOccurrence &occurrence)
{
    // FIXME: implement default operation in argument parser
    if (m_args.pwd.specifiedOperation()) {
        return;
    }
    // print status when no operation specified
    printPwdStatus(occurrence);
}

void Application::printPwdStatus(const ArgumentOccurrence &)
{
    if (!findPwd()) {
        return;
    }
    printDir(RelevantDir{ m_pwd });
    QCoreApplication::quit();
}

void Application::requestRescanPwd(const ArgumentOccurrence &)
{
    if (!findPwd()) {
        return;
    }

    m_pwd.notifyAboutRescan();
    m_connection.rescan(m_pwd.dirObj->id, m_pwd.subDir);
    connect(&m_connection, &SyncthingConnection::rescanTriggered, this, &Application::handleResponse);
    m_expectedResponse = 1;
}

void Application::requestPausePwd(const ArgumentOccurrence &)
{
    if (!findPwd()) {
        return;
    }
    if (m_connection.pauseDirectories(QStringList(m_pwd.dirObj->id))) {
        cerr << "Request pausing directory \"" << m_pwd.dirObj->path.toLocal8Bit().data() << "\" ..." << endl;
        connect(&m_connection, &SyncthingConnection::directoryPauseTriggered, this, &Application::handleResponse);
        m_preventDisconnect = true;
        m_expectedResponse = 1;
    } else {
        cerr << "Directory \"" << m_pwd.dirObj->path.toLocal8Bit().data() << " already paused" << endl;
        QCoreApplication::quit();
    }
}

void Application::requestResumePwd(const ArgumentOccurrence &)
{
    if (!findPwd()) {
        return;
    }
    if (m_connection.resumeDirectories(QStringList(m_pwd.dirObj->id))) {
        cerr << "Request resuming directory \"" << m_pwd.dirObj->path.toLocal8Bit().data() << "\" ..." << endl;
        connect(&m_connection, &SyncthingConnection::directoryResumeTriggered, this, &Application::handleResponse);
        m_preventDisconnect = true;
        m_expectedResponse = 1;
        return;
    } else {
        cerr << "Directory \"" << m_pwd.dirObj->path.toLocal8Bit().data() << " not paused" << endl;
        QCoreApplication::quit();
    }
}

void Application::initDirCompletion(Argument &arg, const ArgumentOccurrence &)
{
    // prevent this initialization if we're not in shell completion mode
    if (m_argsRead) {
        return;
    }
    // load config and wait for connected
    loadConfig();
    waitForConnected(2000);
    // set directory IDs as completion values
    arg.setPreDefinedCompletionValues(m_connection.directoryIds().join(QChar(' ')).toUtf8().data());
}

void Application::initDevCompletion(Argument &arg, const ArgumentOccurrence &)
{
    // prevent this initialization if we're not in shell completion mode
    if (m_argsRead) {
        return;
    }
    // load config and wait for connected
    loadConfig();
    waitForConnected(2000);
    // set device IDs and names as completion values
    QStringList completionValues;
    const size_t valueCount = m_connection.devInfo().size() << 2;
    if (valueCount > numeric_limits<int>::max()) {
        return;
    }
    completionValues.reserve(static_cast<int>(valueCount));
    for (const SyncthingDev &dev : m_connection.devInfo()) {
        completionValues << dev.id << dev.name;
    }
    arg.setPreDefinedCompletionValues(completionValues.join(QChar(' ')).toUtf8().data());
}

RelevantDir Application::findDirectory(const QString &dirIdentifier)
{
    int dummy;
    RelevantDir relevantDir;

    // check whether the specified identifier is a known Syncthing directory or a relative path to an item in a
    // known Syncthing directory
    int firstSlash = dirIdentifier.indexOf(QChar('/'));
    relevantDir.dirObj = m_connection.findDirInfo(firstSlash >= 0 ? dirIdentifier.mid(0, firstSlash) : dirIdentifier, dummy);
    if (relevantDir) {
        if (firstSlash >= 0) {
            relevantDir.subDir = dirIdentifier.mid(firstSlash + 1);
        }
        return relevantDir;
    }

    // check whether the specified identifier is an absolute or relative path of an item inside a known Syncthing directory
    relevantDir.dirObj = m_connection.findDirInfoByPath(
        QDir::isRelativePath(dirIdentifier) ? QDir::currentPath() % QChar('/') % dirIdentifier : dirIdentifier, relevantDir.subDir, dummy);
    if (relevantDir) {
        return relevantDir;
    }

    cerr << Phrases::Warning << "Specified directory \"" << dirIdentifier.toLocal8Bit().data() << "\" is no Syncthing directory (or not part of any)."
         << Phrases::End;
    return relevantDir;
}

void RelevantDir::notifyAboutRescan() const
{
    if (subDir.isEmpty()) {
        cerr << "Request rescanning directory \"" << dirObj->path.toLocal8Bit().data() << "\" ..." << endl;
    } else {
        cerr << "Request rescanning item \"" << subDir.toLocal8Bit().data() << "\" in directory \"" << dirObj->path.toLocal8Bit().data() << "\" ..."
             << endl;
    }
}

} // namespace Cli
