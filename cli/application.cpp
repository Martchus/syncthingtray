#include "./application.h"
#include "./helper.h"

#include "../connector/syncthingconfig.h"

#include <c++utilities/application/failure.h>
#include <c++utilities/io/ansiescapecodes.h>
#include <c++utilities/chrono/timespan.h>
#include <c++utilities/conversion/stringconversion.h>

#include <qtutilities/misc/conversion.h>

#include <QCoreApplication>
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

inline QString argToQString(const char *arg)
{
#if !defined(PLATFORM_WINDOWS)
    return QString::fromLocal8Bit(arg);
#else
    // under Windows args are converted to UTF-8
    return QString::fromUtf8(arg);
#endif
}

Application::Application() :
    m_expectedResponse(0)
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
    m_args.pause.setCallback(bind(&Application::requestPause, this, _1));
    m_args.pauseAll.setCallback(bind(&Application::requestPauseAll, this, _1));
    m_args.resume.setCallback(bind(&Application::requestResume, this, _1));
    m_args.resumeAll.setCallback(bind(&Application::requestResumeAll, this, _1));
    m_args.waitForIdle.setCallback(bind(&Application::initWaitForIdle, this, _1));

    // connect signals and slots
    connect(&m_connection, &SyncthingConnection::statusChanged, this, &Application::handleStatusChanged);
    connect(&m_connection, &SyncthingConnection::error, this, &Application::handleError);
}

Application::~Application()
{}

int Application::exec(int argc, const char * const *argv)
{
    try {
        // parse arguments
        m_args.parser.readArgs(argc, argv);

        // check whether application needs to be terminated due to --bash-completion argument
        if(terminated) {
            return statusCode;
        }

        m_args.parser.checkConstraints();

        // handle help argument
        if(m_args.help.isPresent()) {
            m_args.parser.printHelp(cout);
            return 0;
        }

        // locate and read Syncthing config file
        QString configFile;
        const char *configFileArgValue = m_args.configFile.firstValue();
        if(configFileArgValue) {
            configFile = fromNativeFileName(configFileArgValue);
        } else {
            configFile = SyncthingConfig::locateConfigFile();
        }
        SyncthingConfig config;
        const char *apiKeyArgValue = m_args.apiKey.firstValue();
        if(!config.restore(configFile)) {
            if(configFileArgValue) {
                cerr << "Error: Unable to locate specified Syncthing config file \"" << configFileArgValue << "\"" << endl;
                return -1;
            } else if(!apiKeyArgValue) {
                cerr << "Error: Unable to locate Syncthing config file and no API key specified" << endl;
                return -2;
            }
        }

        // apply settings for connection
        if(const char *urlArgValue = m_args.url.firstValue()) {
            m_settings.syncthingUrl = argToQString(urlArgValue);
        } else if(!config.guiAddress.isEmpty()) {
            m_settings.syncthingUrl = config.syncthingUrl();
        } else {
            m_settings.syncthingUrl = QStringLiteral("http://localhost:8080");
        }
        if(m_args.credentials.isPresent()) {
            m_settings.authEnabled = true;
            m_settings.userName = argToQString(m_args.credentials.values(0)[0]);
            m_settings.password = argToQString(m_args.credentials.values(0)[1]);
        }
        if(apiKeyArgValue) {
            m_settings.apiKey.append(apiKeyArgValue);
        } else {
            m_settings.apiKey.append(config.guiApiKey);
        }
        if(const char *certArgValue = m_args.certificate.firstValue()) {
            m_settings.httpsCertPath = argToQString(certArgValue);
            if(m_settings.httpsCertPath.isEmpty() || !m_settings.loadHttpsCert()) {
                cerr << "Error: Unable to load specified certificate \"" << m_args.certificate.firstValue() << "\"" << endl;
                return -3;
            }
        }

        // finally to request / establish connection
        if(m_args.status.isPresent() || m_args.rescanAll.isPresent() || m_args.pauseAll.isPresent() || m_args.resumeAll.isPresent() || m_args.waitForIdle.isPresent()) {
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

    } catch(const Failure &ex) {
        cerr << "Unable to parse arguments. " << ex.what() << "\nSee --help for available commands." << endl;
        return 1;
    }
}

void Application::handleStatusChanged(SyncthingStatus newStatus)
{
    Q_UNUSED(newStatus)
    if(m_connection.isConnected()) {
        eraseLine(cout);
        cout << '\r';
        m_args.parser.invokeCallbacks();
        if(!m_args.waitForIdle.isPresent()) {
            m_connection.disconnect();
        }
    }
}

void Application::handleResponse()
{
    if(m_expectedResponse) {
        if(!--m_expectedResponse) {
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

void Application::requestLog(const ArgumentOccurrence &)
{
    m_connection.requestLog(bind(&Application::printLog, this, _1));
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
    for(const char *value : occurrence.values) {
        cerr << "Request rescanning " << value << " ...\n";
        m_connection.rescan(argToQString(value));
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

void Application::requestPause(const ArgumentOccurrence &occurrence)
{
    m_expectedResponse = occurrence.values.size();
    connect(&m_connection, &SyncthingConnection::pauseTriggered, this, &Application::handleResponse);
    for(const char *value : occurrence.values) {
        cerr << "Request pausing " << value << " ...\n";
        m_connection.pauseDevice(argToQString(value));
    }
    cerr.flush();
}

void Application::requestPauseAll(const ArgumentOccurrence &)
{
    m_expectedResponse = m_connection.devInfo().size();
    connect(&m_connection, &SyncthingConnection::pauseTriggered, this, &Application::handleResponse);
    cerr << "Request pausing all devices ..." << endl;
    m_connection.pauseAllDevs();
}

void Application::requestResume(const ArgumentOccurrence &occurrence)
{
    m_expectedResponse = occurrence.values.size();
    connect(&m_connection, &SyncthingConnection::resumeTriggered, this, &Application::handleResponse);
    for(const char *value : occurrence.values) {
        cerr << "Request resuming " << value << " ...\n";
        m_connection.resumeDevice(argToQString(value));
    }
    cerr.flush();
}

void Application::requestResumeAll(const ArgumentOccurrence &)
{
    m_expectedResponse = m_connection.devInfo().size();
    connect(&m_connection, &SyncthingConnection::resumeTriggered, this, &Application::handleResponse);
    cerr << "Request resuming all devices ..." << endl;
    m_connection.resumeAllDevs();
}

void Application::findRelevantDirsAndDevs()
{
    int dummy;
    if(m_args.dir.isPresent()) {
        m_relevantDirs.reserve(m_args.dir.occurrences());
        for(size_t i = 0; i != m_args.dir.occurrences(); ++i) {
            if(const SyncthingDir *dir = m_connection.findDirInfo(argToQString(m_args.dir.values(i).front()), dummy)) {
                m_relevantDirs.emplace_back(dir);
            } else {
                cerr << "Warning: Specified directory \"" << m_args.dir.values(i).front() << "\" does not exist and will be ignored" << endl;
            }
        }
    }
    if(m_args.dev.isPresent()) {
        m_relevantDevs.reserve(m_args.dev.occurrences());
        for(size_t i = 0; i != m_args.dev.occurrences(); ++i) {
            const SyncthingDev *dev = m_connection.findDevInfo(argToQString(m_args.dev.values(i).front()), dummy);
            if(!dev) {
                dev = m_connection.findDevInfoByName(argToQString(m_args.dev.values(i).front()), dummy);
            }
            if(dev) {
                m_relevantDevs.emplace_back(dev);
            } else {
                cerr << "Warning: Specified device \"" << m_args.dev.values(i).front() << "\" does not exist and will be ignored" << endl;
            }
        }
    }
    if(m_relevantDirs.empty() && m_relevantDevs.empty()) {
        m_relevantDirs.reserve(m_connection.dirInfo().size());
        for(const SyncthingDir &dir : m_connection.dirInfo()) {
            m_relevantDirs.emplace_back(&dir);
        }
        m_relevantDevs.reserve(m_connection.devInfo().size());
        for(const SyncthingDev &dev : m_connection.devInfo()) {
            m_relevantDevs.emplace_back(&dev);
        }
    }
}

void Application::printStatus(const ArgumentOccurrence &)
{
    findRelevantDirsAndDevs();

    // display dirs
    if(!m_relevantDirs.empty()) {
        setStyle(cout, TextAttribute::Bold);
        cout << "Directories\n";
        setStyle(cout);
        for(const SyncthingDir *dir : m_relevantDirs) {
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
            if(!dir->errors.empty()) {
                cout << "   Errors\n";
                for(const SyncthingDirError &error : dir->errors) {
                    printProperty(" - Message", error.message);
                    printProperty("   File", error.path);
                }
            }
            cout << '\n';
        }
    }

    // display devs
    if(!m_relevantDevs.empty()) {
        setStyle(cout, TextAttribute::Bold);
        cout << "Devices\n";
        setStyle(cout);
        for(const SyncthingDev *dev : m_relevantDevs) {
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
            if(dev->totalIncomingTraffic > 0) {
                printProperty("Incoming traffic", dataSizeToString(static_cast<uint64>(dev->totalIncomingTraffic)).data());
            }
            if(dev->totalOutgoingTraffic > 0) {
                printProperty("Outgoing traffic", dataSizeToString(static_cast<uint64>(dev->totalOutgoingTraffic)).data());
            }
            cout << '\n';
        }
    }

    cout.flush();
    QCoreApplication::exit();
}

void Application::printLog(const std::vector<SyncthingLogEntry> &logEntries)
{
    eraseLine(cout);
    cout << '\r';

    for(const SyncthingLogEntry &entry : logEntries) {
        cout << DateTime::fromIsoStringLocal(entry.when.toLocal8Bit().data()).toString(DateTimeOutputFormat::DateAndTime, true).data() << ':' << ' ' << entry.message.toLocal8Bit().data() << '\n';
    }
    cout.flush();
    QCoreApplication::exit();
}

void Application::initWaitForIdle(const ArgumentOccurrence &)
{
    findRelevantDirsAndDevs();

    // might idle already
    waitForIdle();

    // currently not idling
    // -> relevant dirs/devs might be invalidated so findRelevantDirsAndDevs() must invoked again
    connect(&m_connection, &SyncthingConnection::newDirs, this, &Application::findRelevantDirsAndDevs);
    connect(&m_connection, &SyncthingConnection::newDevices, this, &Application::findRelevantDirsAndDevs);
    // -> check for idle again when dir/dev status changed
    connect(&m_connection, &SyncthingConnection::dirStatusChanged, this, &Application::waitForIdle);
    connect(&m_connection, &SyncthingConnection::devStatusChanged, this, &Application::waitForIdle);
}

void Application::waitForIdle()
{
    for(const SyncthingDir *dir : m_relevantDirs) {
        switch(dir->status) {
        case SyncthingDirStatus::Unknown:
        case SyncthingDirStatus::Idle:
        case SyncthingDirStatus::Unshared:
            break;
        default:
            return;
        }
    }
    for(const SyncthingDev *dev : m_relevantDevs) {
        switch(dev->status) {
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

} // namespace Cli
