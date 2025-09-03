#include "./syncthingtestinstance.h"
#include "./helper.h"

#include <qtutilities/misc/compat.h>
#include <qtutilities/misc/conversion.h>

#include <c++utilities/conversion/conversionexception.h>
#include <c++utilities/conversion/stringbuilder.h>
#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/io/misc.h>
#include <c++utilities/tests/testutils.h>

#include <QDir>
#include <QFileInfo>

#include <functional>
#include <stdexcept>

using namespace std;
using namespace std::placeholders;

namespace CppUtilities {

static int dummy1 = 0;
static char *dummy2;

SyncthingTestInstance::SyncthingTestInstance()
    : m_apiKey(QStringLiteral("syncthingtestinstance"))
    , m_app(dummy1, &dummy2)
    , m_interleavedOutput(false)
    , m_processSupposedToRun(false)
{
    QObject::connect(&m_syncthingProcess, &Data::SyncthingProcess::errorOccurred, &m_syncthingProcess,
        std::bind(&SyncthingTestInstance::handleError, this, _1), Qt::QueuedConnection);
    QObject::connect(&m_syncthingProcess, &Data::SyncthingProcess::finished, &m_syncthingProcess,
        std::bind(&SyncthingTestInstance::handleFinished, this, _1, _2), Qt::QueuedConnection);
}

/*!
 * \brief Starts the Syncthing test instance.
 */
void SyncthingTestInstance::start()
{
    cerr << "\n - Setup configuration for Syncthing tests ..." << endl;

    // set timeout factor for helper
    if (const auto timeoutFactorEnv = qgetenv("SYNCTHING_TEST_TIMEOUT_FACTOR"); !timeoutFactorEnv.isEmpty()) {
        try {
            timeoutFactor = stringToNumber<double>(timeoutFactorEnv.data());
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
    const auto tempDir = tempDirectory();
    const auto relativeConfigFilePath = "testconfig/config.xml"s;
    const auto configFilePathTemplate = testFilePath(relativeConfigFilePath);
    const auto configFilePath = workingCopyPath(relativeConfigFilePath, WorkingCopyMode::NoCopy);
    if (configFilePathTemplate.empty() || configFilePath.empty()) {
        throw runtime_error("Unable to setup Syncthing config directory.");
    }
    auto configFile = readFile(configFilePathTemplate);
    findAndReplace(configFile, "/tmp/", tempDir);
    writeFile(configFilePath, configFile);

    // clean config dir
    const auto configFilePathFileInfo = QFileInfo(QtUtilities::fromNativeFileName(configFilePath));
    const auto configDir = QDir(configFilePathFileInfo.dir());
    for (const auto &configEntry : configDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        if (configEntry.isDir()) {
            QDir(configEntry.absoluteFilePath()).removeRecursively();
        } else if (configEntry.fileName() != QLatin1String("config.xml")) {
            QFile::remove(configEntry.absoluteFilePath());
        }
    }

    // ensure dirs exist
    const auto parentDir = QDir(QtUtilities::fromNativeFileName(tempDir) + QStringLiteral("some/path"));
    parentDir.mkpath(QStringLiteral("1"));
    parentDir.mkpath(QStringLiteral("2"));

    // determine st path
    auto syncthingPath = qEnvironmentVariable("SYNCTHING_PATH");
    if (syncthingPath.isEmpty()) {
        syncthingPath = QStringLiteral("syncthing");
    }

    // determine st port
    const auto syncthingPortFromEnv = qEnvironmentVariableIntValue("SYNCTHING_PORT");
    m_syncthingPort = !syncthingPortFromEnv ? QStringLiteral("4001") : QString::number(syncthingPortFromEnv);

    // start st
    // clang-format off
    const auto args = QStringList{
        QStringLiteral("serve"),
        QStringLiteral("--gui-address=http://127.0.0.1:") + m_syncthingPort,
        QStringLiteral("--gui-apikey=") + m_apiKey,
        QStringLiteral("--home=") + configFilePathFileInfo.absolutePath(),
        QStringLiteral("--no-browser"),
    };
    // clang-format on
    cerr << "\n - Launching Syncthing: " << syncthingPath.toStdString() << ' ' << args.join(QChar(' ')).toStdString() << endl;
    m_processSupposedToRun = true;
    m_syncthingProcess.start(syncthingPath, args);
}

/*!
 * \brief Terminates Syncthing and prints stdout/stderr from Syncthing.
 */
void SyncthingTestInstance::stop()
{
    m_processSupposedToRun = false;
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

/*!
 * \brief Throws an exception to abort testing when the process cannot be started.
 */
void SyncthingTestInstance::handleError(QProcess::ProcessError error)
{
    Q_UNUSED(error)
    throw std::runtime_error(argsToString("Unable to start Syncthing ("sv, m_syncthingProcess.errorString().toStdString(), ')'));
}

/*!
 * \brief Throws an exception to abort testing when the process exists unexpectedly.
 */
void SyncthingTestInstance::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_processSupposedToRun) {
        throw std::runtime_error(argsToString("Syncthing exited unexpectedly ("sv,
            exitStatus == QProcess::NormalExit ? "normal exit"sv : "exited with error"sv, ", exit code: "sv, exitCode, ')'));
    }
}

} // namespace CppUtilities
