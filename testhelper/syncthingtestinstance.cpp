#include "./syncthingtestinstance.h"
#include "./helper.h"

#include <c++utilities/conversion/conversionexception.h>
#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/tests/testutils.h>

#include <QDir>
#include <QFileInfo>

using namespace std;

namespace CppUtilities {

static int dummy1 = 0;
static char *dummy2;

SyncthingTestInstance::SyncthingTestInstance()
    : m_apiKey(QStringLiteral("syncthingtestinstance"))
    , m_app(dummy1, &dummy2)
    , m_interleavedOutput(false)
{
}

/*!
 * \brief Starts the Syncthing test instance.
 */
void SyncthingTestInstance::start()
{
    cerr << "\n - Setup configuration for Syncthing tests ..." << endl;

    // set timeout factor for helper
    const QByteArray timeoutFactorEnv(qgetenv("SYNCTHING_TEST_TIMEOUT_FACTOR"));
    if (!timeoutFactorEnv.isEmpty()) {
        try {
            timeoutFactor = stringToNumber<double>(string(timeoutFactorEnv.data()));
            cerr << " - Using timeout factor " << timeoutFactor << endl;
        } catch (const ConversionException &) {
            cerr << " - Specified SYNCTHING_TEST_TIMEOUT_FACTOR \"" << timeoutFactorEnv.data()
                 << "\" is no valid double and hence ignored\n   (defaulting to " << timeoutFactor << ')' << endl;
        }
    } else {
        cerr << " - No timeout factor set, defaulting to " << timeoutFactor
             << ("\n   (set environment variable SYNCTHING_TEST_TIMEOUT_FACTOR to specify a factor to run tests on a slow machine)") << endl;
    }

    // setup st config
    const string configFilePath = workingCopyPath("testconfig/config.xml");
    if (configFilePath.empty()) {
        throw runtime_error("Unable to setup Syncthing config directory.");
    }
    const QFileInfo configFile(QString::fromLocal8Bit(configFilePath.data()));
    // clean config dir
    const QDir configDir(configFile.dir());
    for (QFileInfo &configEntry : configDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        if (configEntry.isDir()) {
            QDir(configEntry.absoluteFilePath()).removeRecursively();
        } else if (configEntry.fileName() != QStringLiteral("config.xml")) {
            QFile::remove(configEntry.absoluteFilePath());
        }
    }

    // ensure dirs exist
    const QDir parentDir(QStringLiteral("/tmp/some/path"));
    parentDir.mkpath(QStringLiteral("1"));
    parentDir.mkpath(QStringLiteral("2"));

    // determine st path
    const QByteArray syncthingPathFromEnv(qgetenv("SYNCTHING_PATH"));
    const QString syncthingPath(syncthingPathFromEnv.isEmpty() ? QStringLiteral("syncthing") : QString::fromLocal8Bit(syncthingPathFromEnv));

    // determine st port
    const int syncthingPortFromEnv(qEnvironmentVariableIntValue("SYNCTHING_PORT"));
    m_syncthingPort = !syncthingPortFromEnv ? QStringLiteral("4001") : QString::number(syncthingPortFromEnv);

    // start st
    // clang-format off
    const QStringList args{
        QStringLiteral("-gui-address=http://127.0.0.1:") + m_syncthingPort,
        QStringLiteral("-gui-apikey=") + m_apiKey,
        QStringLiteral("-home=") + configFile.absolutePath(),
        QStringLiteral("-no-browser"),
        QStringLiteral("-verbose"),
    };
    cerr << "\n - Launching Syncthing: "
         << syncthingPath.toStdString()
         << ' ' << args.join(QChar(' ')).toStdString() << endl;
    m_syncthingProcess.start(syncthingPath, args);
    // clang-format on
}

/*!
 * \brief Terminates Syncthing and prints stdout/stderr from Syncthing.
 */
void SyncthingTestInstance::stop()
{
    if (m_syncthingProcess.isRunning()) {
        cerr << "\n - Waiting for Syncthing to terminate ..." << endl;
        m_syncthingProcess.terminate();
        m_syncthingProcess.waitForFinished();
    }
    if (m_syncthingProcess.isOpen()) {
        cerr << "\n - Syncthing terminated with exit code " << m_syncthingProcess.exitCode() << ".\n";
        const auto output = m_syncthingProcess.readAll();
        if (!output.isEmpty()) {
            cerr << "\n - Syncthing output (merged stdout/stderr) during the testrun:\n"
                 << std::string_view(output.data(), static_cast<std::string_view::size_type>(output.size()));
        }
        if (!output.isEmpty()) {
            cerr << "\n - Syncthing (re)started: " << output.count("INFO: Starting syncthing") << " times";
            cerr << "\n - Syncthing exited:      " << output.count("INFO: Syncthing exited: exit status") << " times";
            cerr << "\n - Syncthing panicked:    " << output.count("WARNING: Panic detected") << " times";
        }
    }
}

/*!
 * \brief Sets whether Syncthing's output should be forwarded to see what Syncthing and the test is doing at the same time.
 */
void SyncthingTestInstance::setInterleavedOutputEnabled(bool interleavedOutputEnabled)
{
    if (interleavedOutputEnabled == m_interleavedOutput) {
        return;
    }
    m_interleavedOutput = interleavedOutputEnabled;
    m_syncthingProcess.setProcessChannelMode(interleavedOutputEnabled ? QProcess::ForwardedChannels : QProcess::MergedChannels);
}

/*!
 * \brief Applies the default for isInterleavedOutputEnabled() considering environment variable SYNCTHING_TEST_NO_INTERLEAVED_OUTPUT.
 */
void SyncthingTestInstance::setInterleavedOutputEnabledFromEnv()
{
    if (qEnvironmentVariableIsEmpty("SYNCTHING_TEST_NO_INTERLEAVED_OUTPUT")) {
        setInterleavedOutputEnabled(true);
    }
}
} // namespace CppUtilities
